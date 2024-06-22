#pragma once
// 万能类 any
// 2024-6-22
#include <memory>
#include <exception>
#include <cstdio>
#include <typeinfo>

namespace mater 
{
	/* 异常 */
	class bad_cast : public std::exception {
		// 只需要重写 what 函数
		auto what() const noexcept -> const char* override {
			return "bad cast exception!";
		}
	};

	/* 仅仅用于标识该类可以用于安全复制 */
	class copyable {
	public:
		copyable(const copyable&) = default;
		copyable& operator=(const copyable&) = default;
		copyable() = default;
		~copyable() = default;
	};

	class any : public copyable {
	private:
		/* 类型擦除的实现：虚函数多态，子类模板，父类非模板 */
		struct type_base {
			type_base() = default;
			virtual ~type_base() = default;
			/* 获取类型 */
			virtual const std::type_info& get_type() const = 0;
			/* 克隆 */
			virtual std::unique_ptr<type_base> clone() const = 0;
		};

		template <typename T>
		struct type_impl : public type_base {
			T data_;    // 数据成员，通过T传入具体的类型，完成类型擦除

			explicit type_impl(const T& data)
				: data_(data)
			{
			}

			explicit type_impl(T&& data)
				: data_(std::move(data))
			{
			}

			/* 可变参数的移动构造 */
			template<class... Args>
			explicit type_impl(Args&&... args)
				: data_(std::forward<Args>(args)...)
			{
			}

			auto get_type() const  -> const std::type_info & override
			{
				return typeid(T);
			}

			auto clone() const -> std::unique_ptr<type_base> override
			{
				/* 用父类指针接受子类构造出来的对象 */
				return std::unique_ptr<type_impl>(new type_impl(data_));
			}
		};

		public:
			any() = default;
			~any() = default;
			any(const any& other)
			{
				if (other.m_data)
				{
					m_data = other.m_data->clone();
				}
			}

			auto operator=(const any& other) -> any&
			{
				if (other.m_data)
				{
					m_data = other.m_data->clone();
				}
				return *this;
			}

			any(any&& other) noexcept
				: m_data(std::move(other.m_data))
			{
			}

			auto operator=(any&& other) noexcept -> any&
			{
				m_data.swap(other.m_data);
				return *this;
			}

			/* decay的作用是去掉 T 的const 和 & ，让类似于 const T& 直接变成 T */
			template<class T>
			using decay = typename std::decay<T>::type;
			template<class T, typename std::enable_if<!std::is_same<decay<T>, any>::value, bool>::type = true>
			any(T&& data)
				: m_data(new type_impl<decay<T>>(std::forward<T>(data)))
			{
			}

			template<class T, typename std::enable_if<!std::is_same<decay<T>, any>::value, bool>::type = true>
			auto operator=(T&& data) -> any&
			{
				m_data.reset(new type_impl<decay<T>>(std::forward<T>(data)));
				return *this;
			}

			auto has_value() const -> bool // 返回 m_data对象 是否为真，即是否有 m_data 这个对象
			{
				return m_data.operator bool();
			}

			// 返回 m_data 的实际类型，其实是 T 传进来的，这是一个虚函数的多态
			auto type() const -> const std::type_info&
			{
				return has_value() ? m_data->get_type() : typeid(void);
			}

			void reset() noexcept
			{
				m_data.reset();
			}

			void swap(any& other)
			{
				m_data.swap(other.m_data);
			}

			template<class T, class... Args>
			void emplace(Args&&... args)
			{//m_data 智能指针指向 reset(new object) 里的新对象
				m_data.reset(new type_impl<decay<T>>(std::forward<Args>(args)...));
			}

			template<class T>
			const T& cast() const
			{
				if (check_type<T>() && check_not_null())
					return static_cast<const type_impl<T>*>(m_data.get())->data_;
				throw bad_cast();
			}

			template<class T>
			T& cast()
			{
				if (check_type<T>() && check_not_null())
					return static_cast<type_impl<T>*>(m_data.get())->data_;
				throw bad_cast();
			}

		private:
			/* 检查类型是否一致 */
		    template<class T>
		    auto check_type() const -> bool
		    {
			    if (type().hash_code() != typeid(T).hash_code())
			    {
				    fprintf(stderr, "in file %s:%d\n%s => %s type cast error!\n", __FILE__, __LINE__, type().name(), typeid(T).name());
				    return false;
			    }
			    return true;
		    }

			/* 检查数据成员是否存在 */
		    auto check_not_null() const -> bool
		    {
			    if (!m_data)
			    {
				    fprintf(stderr, "in file %s:%d\n nullptr data", __FILE__, __LINE__);
				    return false;
			    }
			    return true;
		    }

		    std::unique_ptr<type_base> m_data;
	};

	inline void swap(any& lhs, any& rhs) { lhs.swap(rhs); }

	template <class T> auto any_cast(const any& any) -> const T& {
		return any.template cast<T>();
	}

	template <class T> auto any_cast(any& any) -> T& {
		return any.template cast<T>();  // 这是直接调用 类里面的 cast 吗？看不懂....
	}
}