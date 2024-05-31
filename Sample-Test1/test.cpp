#include "../Project16/BookingScheduler.cpp"
#include "TestableSmsSender.cpp"
#include "TestableMailSender.cpp"
#include "TestableBookingScheduler.cpp"
#include "gtest/gtest.h"

class MockCustomer: public Customer{
public:
	MOCK_METHOD(string, getEmail, (), (override));
}


class BookingItem : public testing::Test {
protected:
	void SetUp() override {
		NOT_ON_THE_HOUR = getTime(2021, 3, 26, 9, 5);
		ON_THE_HOUR = getTime(2021, 3, 26, 9, 0);
		SUNDAY_ON_THE_HOUR = getTime(2021, 3, 28, 17, 0);
		MONDAY_ON_THE_HOUR = getTime(2024, 6, 3, 17, 0);
		bookingScheduler.setSmsSender(&testableSmsSender);
		bookingScheduler.setMailSender(&testableMailSender);
	}

public:
	tm getTime(int year, int mon, int day, int hour, int min) {
		tm result = { 0, min, hour, day, mon - 1, year - 1900, 0, 0, -1 };
		mktime(&result);
		return result;
	}

	tm plusHour(tm base, int hour) {
		base.tm_hour += hour;
		mktime(&base);
		return base;
	}

	tm NOT_ON_THE_HOUR;
	tm ON_THE_HOUR;
	tm MONDAY_ON_THE_HOUR;
	tm SUNDAY_ON_THE_HOUR;
	Customer CUSTOMER{ "Fake name", "010-1234-5678" };
	Customer CUSTOMER_WITH_MAIL{ "Fake Name", "010-1234-5678", "test@test.com" };

	const int UNDER_CAPACITY = 1;
	const int CAPACITY_PER_HOUR = 3;

	BookingScheduler bookingScheduler{ CAPACITY_PER_HOUR };
	TestableSmsSender testableSmsSender;
	TestableMailSender testableMailSender;
};

TEST_F(BookingItem, 예약은_정시에만_가능하다_정시가_아닌경우_예약불가) {
	//arrange
	Schedule* schedule = new Schedule{ NOT_ON_THE_HOUR, UNDER_CAPACITY, CUSTOMER };

	//act
	EXPECT_THROW({
			bookingScheduler.addSchedule(schedule);
		}, std::runtime_error);

	//assert
	//expected runtime exception
}

TEST_F(BookingItem, 예약은_정시에만_가능하다_정시인_경우_예약가능) {
	//arrange
	Schedule* schedule = new Schedule{ ON_THE_HOUR, UNDER_CAPACITY, CUSTOMER };

	//act
	bookingScheduler.addSchedule(schedule);

	//assert
	EXPECT_EQ(true, bookingScheduler.hasSchedule(schedule));
}


TEST_F(BookingItem, 시간대별_인원제한이_있다_같은_시간대에_Capacity_초과할_경우_예외발생) {
	//arrange
	Schedule* schedule = new Schedule{ ON_THE_HOUR, CAPACITY_PER_HOUR, CUSTOMER };
	bookingScheduler.addSchedule(schedule);

	//act
	try {
		Schedule* newSchedule = new Schedule{ ON_THE_HOUR, UNDER_CAPACITY, CUSTOMER };
		bookingScheduler.addSchedule(newSchedule);
		FAIL(); //Exception이 발생할 예정이기에, FAIL() 함수에 도달하지 않는다.
	}
	catch (std::runtime_error& e) {
		//assert
		EXPECT_EQ(string{ e.what() }, string{ "Number of people is over restaurant capacity per hour" });
	}
}

TEST_F(BookingItem, 시간대별_인원제한이_있다_같은_시간대가_다르면_Capacity_차있어도_스케쥴_추가_성공) {
	//arrange
	Schedule* schedule = new Schedule{ ON_THE_HOUR, CAPACITY_PER_HOUR, CUSTOMER };
	bookingScheduler.addSchedule(schedule);

	//act
	tm differentHour = plusHour(ON_THE_HOUR, 1);
	Schedule* newSchedule = new Schedule{ differentHour, UNDER_CAPACITY, CUSTOMER };
	bookingScheduler.addSchedule(newSchedule);

	//assert
	EXPECT_EQ(true, bookingScheduler.hasSchedule(schedule));
}

TEST_F(BookingItem, 예약완료시_SMS는_무조건_발송) {
	//arrange
	Schedule* schedule = new Schedule{ ON_THE_HOUR, CAPACITY_PER_HOUR, CUSTOMER };

	//act
	bookingScheduler.addSchedule(schedule);

	//assert
	EXPECT_EQ(true, testableSmsSender.isSendMethodIsCalled());
}

TEST_F(BookingItem, 이메일이_없는_경우에는_이메일_미발송) {
	//arrange
	Schedule* schedule = new Schedule{ ON_THE_HOUR, UNDER_CAPACITY, CUSTOMER };

	//act
	bookingScheduler.addSchedule(schedule);

	//assert
	EXPECT_EQ(0, testableMailSender.getCountSendMailMethodIsCalled());
}

TEST_F(BookingItem, 이메일이_있는_경우에는_이메일_발송) {
	//arrange
	Schedule* schedule = new Schedule{ ON_THE_HOUR, UNDER_CAPACITY, CUSTOMER_WITH_MAIL };

	//act
	bookingScheduler.addSchedule(schedule);

	//assert
	EXPECT_EQ(1, testableMailSender.getCountSendMailMethodIsCalled());
}

TEST_F(BookingItem, 현재날짜가_일요일인_경우_예약불가_예외처리) {
	//arrange
	BookingScheduler* bookingScheduler
		= new TestableBookingScheduler{ CAPACITY_PER_HOUR, SUNDAY_ON_THE_HOUR };

	try {
		//act
		Schedule* schedule = new Schedule{ ON_THE_HOUR, UNDER_CAPACITY, CUSTOMER_WITH_MAIL };
		bookingScheduler->addSchedule(schedule);
		FAIL();
	}
	catch (std::runtime_error& e) {
		//assert
		EXPECT_EQ(string{ e.what() }, string{ "Booking system is not available on sunday" });
	}
}

TEST_F(BookingItem, 현재날짜가_일요일이_아닌경우_예약가능) {
	//arrange
	BookingScheduler* bookingScheduler
		= new TestableBookingScheduler{ CAPACITY_PER_HOUR, MONDAY_ON_THE_HOUR };

	//act
	Schedule* schedule = new Schedule{ ON_THE_HOUR, UNDER_CAPACITY, CUSTOMER_WITH_MAIL };
	bookingScheduler->addSchedule(schedule);

	//assert
	EXPECT_EQ(true, bookingScheduler->hasSchedule(schedule));
}
