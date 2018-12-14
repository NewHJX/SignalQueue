#pragma once
#include<any>
#include<tuple>
#include<map>
#include<mutex>
#include<queue>
#include <optional>
#include <type_traits>
#include <functional>
#include "Event.h"

namespace message {
	namespace internal {
		namespace nonstd {

			template <typename Ty>
			class queue {
			public:
				void push(Ty&& value) {
					std::lock_guard<std::mutex> lock(mutex_);
					queue_.push(std::move(value));
				}

				std::optional<Ty> pop() {
					std::lock_guard<std::mutex> lock(mutex_);
					if (!queue_.empty()) {
						Ty value = std::move(queue_.front());
						queue_.pop();
						return std::move(value);
					}
					return std::optional<Ty>();
				}

				template <typename... Args>
				void emplace(Args&&... args) {
					std::lock_guard<std::mutex> lock(mutex_);
					queue_.emplace(std::forward<Args>(args)...);
				}

			private:
				std::queue<Ty> queue_;
				std::mutex mutex_;
			};

			template <typename Kty, typename Ty, typename Hasher = std::hash<Kty>>
			class map {
			public:
				size_t count(const Kty& key) {
					//std::shared_lock<std::shared_mutex> lock(mutex_);
					return map_.count(key);
				}

				Ty& at(const Kty& key) {
					//std::shared_lock<std::shared_mutex> lock(mutex_);
					return map_.at(key);
				}

				Ty& at(Kty&& key) {
					//std::shared_lock<std::shared_mutex> lock(mutex_);
					return map_.at(key);
				}

				Ty& operator[](const Kty& key) {
					//std::shared_lock<std::shared_mutex> lock(mutex_);
					return map_[key];
				}

				Ty& operator[](Kty&& key) {
					//std::shared_lock<std::shared_mutex> lock(mutex_);
					return map_[key];
				}

				void erase(const Kty& key) {
					map_.erase(key);
				}

				template <typename... Args>
				void emplace(Args&&... args) {
					//std::unique_lock<std::shared_mutex> lock(mutex_);
					map_.emplace(std::forward<Args>(args)...);
				}

			private:
				std::unordered_map<Kty, Ty, Hasher> map_;
				//std::shared_mutex mutex_;
			};

		} // namespace nonstd

		using SignalFunc = std::function<std::any(std::any)>;
		using SignalMap = std::map<size_t, SignalFunc>;

		using CallbackFunc = std::function<void(std::any)>;
		using CallbackMap = std::map<size_t, CallbackFunc>;

		using EventPair = std::pair<size_t, size_t>;
		using EventItem = std::pair<EventPair, std::any>;
		using EventQueue = std::queue<EventItem>;
		using EventMap = std::map<size_t, EventQueue>;

		struct Holder {
			SignalMap signals;
			CallbackMap callbacks;
			EventMap events;
		};

		inline Holder& holder() {
			static Holder holder;
			return holder;
		}

		inline size_t hash(size_t first, size_t second) {
			return first << 16 | second;
		}

		template<size_t Type>
		void handle_event() {
			Holder& holder = internal::holder();
			EventQueue& events = holder.events.at(Type);
			auto& signals = holder.signals;
			auto& callbacks = holder.callbacks;
			while (!events.empty()) {
				auto item = events.front();

				events.pop();
				size_t eid = item.first.first;
				size_t cid = hash(item.first.first, item.first.second);
				auto result = signals.at(eid)(std::move(item.second));
				if (callbacks.count(cid)) {
					callbacks.at(cid)(std::move(result));
				}
			}
		}

		template <size_t Type>
		class Trigger;

		template <>
		class Trigger<MessageType1> {
		public:
			static void trigger() { handle(); }
			static void handle() {
				handle_event<MessageType1>();
			}
		};

		template<>
		class Trigger<MessageType2> {
		public:
			static void trigger() { handle(); }
			static void handle() {
				handle_event<MessageType2>();
			}
		};
	} //internal

	class Signal {
	private:
		template <typename R, typename E, typename F>
		struct invoker {
			static R invoke(const F& f, std::any any) {
				return std::apply(f, std::any_cast<typename E::Params>(std::move(any)));
			}
		};

		template<typename E, typename F>
		struct invoker<void, E, F> {
			static std::any invoke(const F& f, std::any any) {
				std::apply(f, std::any_cast<typename E::Params>(std::move(any)));
				return std::any();
			}
		};
	public:
		template<typename Event, typename Func>
		void enroll(Func func) {
			//这里可以加一些断言

			internal::Holder& holder = internal::holder();
			if (holder.signals.count(Event::value)) {
				holder.signals.erase(Event::value);
			}
			holder.signals.emplace(Event::value, [func = std::move(func)](std::any any){
				return invoker<typename Event::Return, Event, Func>::invoke(func, std::move(any));
			});
		}

		template<typename Event, typename Func>
		void callback(Func func) {
			//
			size_t key = internal::hash(Event::value, reinterpret_cast<size_t>(this));
				internal::Holder& holder = internal::holder();
			if (holder.callbacks.count(key)) {
				holder.callbacks.erase(key);
			}
			holder.callbacks.emplace(key, [func = std::move(func)](std::any any){
				func(std::any_cast<typename Event::Return>(std::move(any)));
			});
		}

		template <typename Event, typename...Args>
		void call(Args&&... args) {
			//
			internal::Holder& holder = internal::holder();
			if (!holder.signals.count(Event::value)) {
				return;
			}
			holder.events[Event::type].emplace(std::make_pair(Event::value, reinterpret_cast<size_t>(this)),
				std::make_tuple(std::forward<Args>(args)...));
			internal::Trigger<Event::type>::trigger();
		}
	};
} //message