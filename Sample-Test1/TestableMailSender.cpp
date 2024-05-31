#include "../Project16/MailSender.cpp"

class TestableMailSender : public MailSender {
public:
	void sendMail(Schedule* schedule) override {
		countSendMailMethodIsCalled++;
	}

	int getCountSendMailMethodIsCalled() {
		return countSendMailMethodIsCalled;
	}

private:
	int countSendMailMethodIsCalled = 0;
};