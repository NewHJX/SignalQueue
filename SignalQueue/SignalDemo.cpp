#include"Signal.h"

void PrintSomeThingForEvent1() {
	printf("Event 1\n");
}
void PrintSomeThingForEvent2() {
	printf("Event 2\n");
}


int main() {
	message::Signal m_Signal;
	m_Signal.enroll<EventMessageType1>([]() { PrintSomeThingForEvent1(); });
	m_Signal.enroll<EventMessageType2>([]() { PrintSomeThingForEvent2(); });
	m_Signal.call<EventMessageType2>();
	m_Signal.call<EventMessageType1>();
	getchar();
	return 0;
}