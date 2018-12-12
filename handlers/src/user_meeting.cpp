#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <handlers.hpp>
#include <nlohmann/json.hpp>
#include <Poco/URI.h>
#include <user_meeting.hpp>
#include <optional>
#include <data_session_factory.hpp>
#include <mutex>

namespace handlers {

using namespace Poco::Data::Keywords;
using nlohmann::json;

class DBStorage : public Storage {
public:
	void Save(Meeting &meeting) override {
		std::lock_guard<std::mutex> lock{m_mutex};
		auto session = DataSessionFactory::getInstance();
		int id;
            if (meeting.id.has_value()) {
				Poco::Data::Statement update(session);
				update << "UPDATE meeting SET "
						"name=?, description=?, address=?, "
						"signup_description=?, signup_from_date=?, "
						"signup_to_date=?, from_date=?, to_date=?, published=?"
						"WHERE id=?",
					use(meeting.name),
					use(meeting.description),
					use(meeting.address),
					use(meeting.signup_description),
					use(meeting.signup_from_date),
					use(meeting.signup_to_date),
					use(meeting.from_date),
					use(meeting.to_date),
					use(meeting.published),
					use(meeting.id.value()),
					now;
			} else {
				Poco::Data::Statement insert(session);
				insert << "INSERT INTO meeting "
						"(name, description, address, signup_description, "
						"signup_from_date, signup_to_date, from_date, to_date, published) "
						"VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?)",
					use(meeting.name),
					use(meeting.description),
					use(meeting.address),
					use(meeting.signup_description),
					use(meeting.signup_from_date),
					use(meeting.signup_to_date),
					use(meeting.from_date),
					use(meeting.to_date),
					use(meeting.published),
					now;

				Poco::Data::Statement select(session);
				select << "SELECT last_insert_rowid() as id FROM meeting LIMIT 1",
						into(id),
						now;
				meeting.id = id;
			}
		session.close();
	}

	void Delete(int id) override {
		std::lock_guard<std::mutex> lock{m_mutex};
		auto session = DataSessionFactory::getInstance();
		Poco::Data::Statement deleteMeeting(session);
		deleteMeeting << "DELETE FROM meeting WHERE id = ?",
				use(id);
		deleteMeeting.execute();
		session.close();
	}

	Meeting Get(int id) override {
		std::lock_guard<std::mutex> lock{m_mutex};
		auto session = DataSessionFactory::getInstance();
		Poco::Data::Statement select(session);
		Meeting meeting;

		select << "SELECT "
					"id, name, description, address, signup_description, "
					"signup_from_date, signup_to_date, from_date, to_date, published "
					"FROM meeting WHERE id=?",
			use(id),
			into(meeting.id.emplace()),
			into(meeting.name),
			into(meeting.description),
			into(meeting.address),
			into(meeting.signup_description),
			into(meeting.signup_from_date),
			into(meeting.signup_to_date),
			into(meeting.from_date),
			into(meeting.to_date),
			into(meeting.published),
			now;
	
		return meeting;
            
	}

	Storage::MeetingList GetList() override {
		std::lock_guard<std::mutex> lock{m_mutex};
		auto session = DataSessionFactory::getInstance();
		Poco::Data::Statement select(session);
		Storage::MeetingList list;
		Meeting meeting;

		select << "SELECT "
			      "id, name, description, address, signup_description, "
		          "signup_from_date, signup_to_date, from_date, to_date, published "
			      "FROM meeting",
			into(meeting.id.emplace()),
			into(meeting.name),
			into(meeting.description),
			into(meeting.address),
			into(meeting.signup_description),
			into(meeting.signup_from_date),
			into(meeting.signup_to_date),
			into(meeting.from_date),
			into(meeting.to_date),
			into(meeting.published),
			range(0, 1); //  iterate over result set one row at a time

		while (!select.done() && select.execute() > 0) {
			list.push_back(meeting);
		}
		return list;
	}

	bool Exists(int id) override {
		auto session = DataSessionFactory::getInstance();
		int meeting_count = 0;
		session << "SELECT COUNT(*) FROM meeting WHERE id=?", use(id), into(meeting_count), now;
		return meeting_count > 0;
	}

private:
	std::mutex m_mutex;

};

Storage &GetStorage() {
	static DBStorage storage;
	return storage;
}


void UserMeetingList::HandleRestRequest(Poco::Net::HTTPServerRequest &request, Poco::Net::HTTPServerResponse &response) {
	response.setStatus(Poco::Net::HTTPServerResponse::HTTP_OK);
	auto &storage = GetStorage();
	nlohmann::json result = nlohmann::json::array();
	for (auto meeting : storage.GetList()) {
		result.push_back(meeting);
	}
	
	response.send() << result;

}

void UserMeetingGet::HandleRestRequest(Poco::Net::HTTPServerRequest &request, Poco::Net::HTTPServerResponse &response) {

	auto &storage = GetStorage();

	if (storage.Exists(m_id)) {
		response.setStatus(Poco::Net::HTTPServerResponse::HTTP_OK);
		response.send() << json(storage.Get(m_id));
	} else {
		meeting::GetLogger().information("Trying to get meeting #"+std::to_string(m_id)+":");
		meeting::GetLogger().information("Not found.");
		response.setStatus(Poco::Net::HTTPServerResponse::HTTP_NOT_FOUND);
		response.send();
	}

}

void UserMeetingCreate::HandleRestRequest(Poco::Net::HTTPServerRequest &request, Poco::Net::HTTPServerResponse &response) {
	
	nlohmann::json j = nlohmann::json::parse(request.stream());
	Meeting meeting = j;

	auto &storage = GetStorage();
	storage.Save(meeting);
	
	response.setStatus(Poco::Net::HTTPServerResponse::HTTP_OK);
	response.send() << json(meeting);
	
	meeting::GetLogger().information("Created new meeting:");
	meeting::GetLogger().information(json(meeting).dump());

}

void UserMeetingUpdate::HandleRestRequest(Poco::Net::HTTPServerRequest &request, Poco::Net::HTTPServerResponse &response) {

	auto &storage = GetStorage();

	if (storage.Exists(m_id)) {

		nlohmann::json j = nlohmann::json::parse(request.stream());
		Meeting meeting = j;
		meeting.id = m_id;
		storage.Save(meeting);
		response.setStatus(Poco::Net::HTTPServerResponse::HTTP_OK);
		response.send() << json(meeting);
		meeting::GetLogger().information("Updated  meeting #"+std::to_string(m_id)+":");
		meeting::GetLogger().information(j.dump());

	} else {
		meeting::GetLogger().information("Trying to update meeting #"+std::to_string(m_id)+":");
		meeting::GetLogger().information("Not found.");
		response.setStatus(Poco::Net::HTTPServerResponse::HTTP_NOT_FOUND);
		response.send();
	}

}


void UserMeetingDelete::HandleRestRequest(Poco::Net::HTTPServerRequest &request, Poco::Net::HTTPServerResponse &response) {

	auto &storage = GetStorage();

	if (storage.Exists(m_id)) {
		storage.Delete(m_id);
		response.setStatus(Poco::Net::HTTPServerResponse::HTTP_NO_CONTENT);
		response.send();
		meeting::GetLogger().information("Deleted  meeting #"+std::to_string(m_id));
	} else {
		meeting::GetLogger().information("Trying to delete meeting #"+std::to_string(m_id)+":");
		meeting::GetLogger().information("Not found.");
		response.setStatus(Poco::Net::HTTPServerResponse::HTTP_NOT_FOUND);
		response.send();
	}
}


} // namespace handlers
