#pragma once

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

#include <curl/curl.h>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

using namespace std;
using namespace boost;
//using namespace google;
using namespace google::protobuf;

typedef multimap<string, string> Form;

inline Form
parseForm(string s) {
	Form form;
	string line;
	stringstream in(s);
	while (getline(in, line, '&')) {
		stringstream tmp(line);
		string key;
		if (getline(tmp, key, '=')) {
			key = shared_ptr<char>(evhttp_decode_uri(key.c_str())).get();
			string value;
			if (getline(tmp, value, '='))
				value = shared_ptr<char>(evhttp_decode_uri(value.c_str())).get();
			form.insert(make_pair(key, value));
		}
	}
	return form;
}

inline string
renderForm(Form form) {
	CURL *handle;
	string result;
	for (std::map<string, string>::iterator iter = form.begin(); iter != form.end(); ++iter) {
		if (iter != form.begin())
			result += "&";
		result += shared_ptr<char>(curl_easy_escape(handle, (*iter).first.c_str(), 0), curl_free).get();
		result += "=";
		result += shared_ptr<char>(curl_easy_escape(handle, (*iter).second.c_str(), 0), curl_free).get();
	}
	return result;
}

/**
 * ProtoReadForm
 *
 * reads from form to protobuf message
 *
 * @param form
 */
inline void
ProtoReadForm(::google::protobuf::Message *message, Form form) {
	for (int index = 0; index < message->GetDescriptor()->field_count(); ++index) {
		const FieldDescriptor *field = message->GetDescriptor()->field(index);
		string name(field->name());
		Form::iterator iter = form.find(name);
		if (iter != form.end()) {
			string value((*iter).second);
			switch (field->cpp_type()) {
			case FieldDescriptor::CPPTYPE_ENUM:
			{
				// e.g., "en-US" -> "EN_US"
				// try to do a case-insensitive lookup
				to_upper(value);
				boost::replace_all(value, "-", "_");
				for (int index = 0; index < field->enum_type()->value_count(); ++index) {
					const EnumValueDescriptor *enumValue = field->enum_type()->value(index);
					string name(enumValue->name());
					to_upper(name);
					boost::replace_all(name, "-", "_");
					if (name==value)
						field->is_repeated()?message->GetReflection()->AddEnum(message, field, enumValue):message->GetReflection()->SetEnum(message, field, enumValue);
				}
				break;
			}
			case FieldDescriptor::CPPTYPE_STRING:
				field->is_repeated()?message->GetReflection()->AddString(message, field, value):message->GetReflection()->SetString(message, field, value);
				break;
			default:
				break;
			}
		}
	}
}

/**
 * ProtoWriteForm
 *
 * writes from protobuf message to form
 *
 * @param form
 */
inline void
ProtoWriteForm(::google::protobuf::Message *message, Form *form) {
	for (int index = 0; index < message->GetDescriptor()->field_count(); ++index) {
		const FieldDescriptor *field = message->GetDescriptor()->field(index);
		string name(field->name());
		if (message->GetReflection()->HasField(*message, field)) {
			switch (field->cpp_type()) {
			case FieldDescriptor::CPPTYPE_STRING:
				form->insert(make_pair(name, message->GetReflection()->GetString(*message, field)));
				break;
			default:
				break;
			}
		}
	}
}
