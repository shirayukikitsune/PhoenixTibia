#pragma once
#include <functional>
#include <list>
#include <map>
#include <algorithm>

namespace phoenix {
	template<bool, class> class callback
	{};

	namespace nonvoid {
		template <bool once, class R, typename... ArgTypes>
		class caller {
		public:
			typedef std::function<R(ArgTypes...)> function_type;

			std::map<int, R> operator()(std::map<int, function_type> &functions, ArgTypes... args) {
				std::map<int, R> r;

				for (auto &&f : functions) {
					r[f.first] = f.second(args...);
				}

				return r;
			}
		};

		template <class R, typename... ArgTypes>
		class caller <true, R, ArgTypes...> {
		public:
			typedef std::function<R(ArgTypes...)> function_type;

			std::map<int, R> operator()(std::map<int, function_type> &functions, ArgTypes... args) {
				std::map<int, R> r;

				for (auto &&f : functions) {
					r[f.first] = f.second(args...);
				}

				functions.clear();

				return r;
			}
		};
	}

	template<bool fireOnce, class R, typename... ArgTypes>
	class callback<fireOnce, R(ArgTypes...)>
	{
	public:
		typedef class callback<fireOnce, R(ArgTypes...)> _Myt;
		typedef R result_type;
		typedef std::function<R(ArgTypes...)> function_type;

	private:
		nonvoid::caller<fireOnce, R, ArgTypes...> caller;
	public:
		callback()
		{
			m_id = 0;
		}

		_Myt& clear() {
			m_id = 0;
			m_functions.clear();
			return *this;
		}

		operator bool() const {
			return !m_functions.empty();
		}

		int push(const function_type &fn) {
			m_functions[++m_id] = fn;

			return m_id;
		}

		int push(function_type && fn) {
			m_functions[++m_id] = fn;

			return m_id;
		}

		_Myt& pop(int index) {
			auto i = m_functions.find(index);
			if (i != m_functions.end()) m_functions.erase(i);

			return *this;
		}

		std::map<int, R> operator() (ArgTypes... args) {
			return caller(m_functions, args...);
		}

		_Myt& operator-= (int index) {
			this->pop(index);

			return *this;
		}

		_Myt& operator+= (function_type && fn) {
			this->push(fn);

			return *this;
		}

		_Myt& operator= (function_type && fn) {
			this->clear();
			this->push(fn);

			return *this;
		}

	private:
		int m_id;
		std::map<int, function_type> m_functions;
	};

	namespace noret {
		template <bool once, typename... ArgTypes>
		class caller {
		public:
			typedef std::function<void(ArgTypes...)> function_type;

			void operator()(std::map<int, function_type> &functions, ArgTypes... args) {
				for (auto &f : functions) {
					f.second(args...);
				}
			}
		};

		template <typename... ArgTypes>
		class caller < true, ArgTypes... > {
		public:
			typedef std::function<void(ArgTypes...)> function_type;

			void operator()(std::map<int, function_type> &functions, ArgTypes... args) {
				for (auto &f : functions) {
					f.second(args...);
				}

				functions.clear();
			}
		};
	}

	template<bool fireOnce, class... ArgTypes>
	class callback<fireOnce, void(ArgTypes...)>
	{
	public:
		typedef class callback<fireOnce, void(ArgTypes...)> _Myt;
		typedef void result_type;
		typedef std::function<void(ArgTypes...)> function_type;

	private:

		noret::caller<fireOnce, ArgTypes...> caller;

	public:
		callback()
		{
			m_id = 0;
		}

		_Myt& clear() {
			m_id = 0;
			m_functions.clear();
			return *this;
		}

		operator bool() const {
			return !m_functions.empty();
		}

		void operator() (ArgTypes... args) {
			caller(m_functions, args...);
		}

		int push(const function_type &fn) {
			m_functions[++m_id] = fn;

			return m_id;
		}

		int push(function_type && fn) {
			m_functions[++m_id] = fn;

			return m_id;
		}

		void pop(int index) {
			auto i = m_functions.find(index);
			if (i != m_functions.end()) m_functions.erase(i);
		}

		_Myt& operator-= (int index) {
			this->pop(index);

			return *this;
		}

		_Myt& operator+= (function_type && fn) {
			this->push(fn);

			return *this;
		}

		_Myt& operator= (function_type && fn) {
			this->clear();
			this->push(fn);

			return *this;
		}

	private:
		int m_id;
		std::map<int, function_type> m_functions;
	};
}
