#pragma once

#include "located_exception.h"
#include <vector>
#include <mutex>
#include <condition_variable>

namespace mvec {

	/// このクラスは複数のスレッドでの処理結果の集計などに用いることができる
	/// thread には 0 ~ use_thread - 1の indexが対応する
	template<typename t>
	class multi_vector {

	public:

		using value_type = t;

	private:

		std::mutex _mtx;
		std::condition_variable _cv;
		std::size_t _reserved = 0;//reserveまで到達したthread
		bool _calculate_end = false;

		std::vector<std::vector<value_type>> _vecs;

		std::vector<size_t> _offsets;

		std::vector<value_type> _totallinged;

	public:

		multi_vector(const size_t& use_thread)
			:_vecs(use_thread)
		{
			static_assert(std::is_trivially_copyable_v<t>, "使用可能なのはトリビアルコピー可能な型のみ");
		}

		/// <summary>
		/// 最初に一つのスレッドから呼び出す
		/// 総数を入力する
		/// 
		/// 各スレッド用に均等にバッファが準備される
		/// </summary>
		/// <param name="count"></param>
		void reserve_push(const size_t& count) {
			if (_vecs.empty()) return;

			auto div_count = (count / _vecs.size()) + 1;
			for (auto& vec : _vecs)
			{
				vec.reserve(div_count);
			}
		}

		/// <summary>
		/// 集計準備 それぞれのスレッドから呼び出され、処理が完了するまでブロッキング
		/// 0に対応するthreadは必ず呼ばなければならない
		/// </summary>
		void reserve_totalling(size_t ind) {

			if (_vecs.empty()) return;

			if (ind == 0)
				_offsets.resize(_vecs.size(), 0);

			std::unique_lock lock(_mtx);
			
			++_reserved;

			if (_reserved == _vecs.size()) {
				_cv.notify_all();
				_offsets[ind] = 1;
			}
			else {

				_cv.wait(lock, [&]() {
					return _reserved == _vecs.size();
					});
				_offsets[ind] = 1;
			}

			//ここで全てのスレッドが足並みを揃えないと一つ上のwaitでブロッキングする可能性がある
			



			if (ind == 0) {

				lock.unlock();
				size_t tmp;
				do
				{
					tmp = 0;
					for (auto&i : _offsets)
					{
						tmp += i;
					}
				} while (tmp != _offsets.size());


				lock.lock();
				size_t sum = 0;
				for (size_t i = 0; i < _vecs.size(); i++)
				{
					_offsets[i] = sum;
					sum += _vecs[i].size();
				}
				_totallinged.resize(sum);

				//終了合図
				_calculate_end = true;

				_cv.notify_all();
			}
			else if (!_calculate_end) {
				//集計が終わるまで他のスレッドが帰ると不味いので止める
				_cv.wait(lock, [&]() {
					return _calculate_end;
					});
			}
		}

		void push_back(const size_t& ind, const value_type& val) {
			_vecs[ind].emplace_back(val);
		}
		void push_back(const size_t& ind, value_type&& val) {
			_vecs[ind].emplace_back(std::move(val));
		}

		/// <summary>
		/// それぞれのthreadから呼び出す
		/// 0に対応するthreadは必ず呼ばなければならない
		/// </summary>
		/// <param name="ind"></param>
		void totalling(const size_t& ind) {

#ifdef _DEBUG
			if (ind >= _offsets.size())
				throw error::located_exception("out of range");
#endif

			if (_totallinged.empty() || _vecs[ind].empty()) return;

			std::memcpy(_totallinged.data() + _offsets[ind], _vecs[ind].data(), _vecs[ind].size() * sizeof(value_type));
			_vecs[ind].clear();

			//終了を知らせる
			std::lock_guard lock(_mtx);
			--_reserved;

			if (_reserved == 0) {
				_cv.notify_all();//waitしているメインスレッドに通知

				_calculate_end = false;
			}
		}


		/// <summary>
		/// totalling が終了するまでブロッキングする
		/// </summary>
		void totalling_wait() {
			std::unique_lock lock(_mtx);
			_cv.wait(
				lock,
				[&]() {
					return _reserved == 0;
				});
		}

		/// <summary>
		/// tottalling後に有効になる
		/// </summary>
		/// <returns></returns>
		value_type* data() {
			return _totallinged.data();
		}
		/// <summary>
		/// tottalling後に有効になる
		/// </summary>
		/// <returns></returns>
		const value_type* data() const {
			return _totallinged.data();
		}

		const size_t size() const {
			return _totallinged.size();
		}
	};
}