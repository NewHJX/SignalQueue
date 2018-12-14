#pragma once
#include <tuple>
namespace message {
	enum { MessageType1, MessageType2 };
	//每个消息事件用type,id,signature来确定
	template <size_t Type, size_t Id, typename Signature>
	struct Event;

	template <size_t Type, size_t Id, typename Result, typename... Args>
	struct Event<Type, Id, Result(Args...)> {
		enum { type = Type };
		enum { value = Id };
		//别名
		using Signature = Result(Args...);
		using Return = Result;
		using Params = std::tuple<Args...>;
	};

	template<size_t Id, typename Signature>
	using MessageType1Event = Event<MessageType1, Id, Signature>;

	template<size_t Id, typename Signature>
	using MessageType2Event = Event<MessageType2, Id, Signature>;
}  //namespace message

using EventMessageType1 = message::MessageType1Event<1, void()>;
using EventMessageType2 = message::MessageType2Event<2, void()>;