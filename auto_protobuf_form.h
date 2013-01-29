#pragma once

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

#include <evhttp.h> // for evhttp_decode_uri

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

using namespace std;
using namespace boost;
//using namespace google;
using namespace google::protobuf;

typedef multimap<string, string> Form;

inline Form parseForm(string s) {
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

/**
 * ProtoReadForm
 *
 * @param form
 */
inline void
ProtoReadForm(::google::protobuf::Message *message, Form form) {
	for (Form::iterator iter = form.begin(); iter!=form.end(); ++iter) {
		string name((*iter).first);
		string value((*iter).second);
		const FieldDescriptor *field = message->GetDescriptor()->FindFieldByName(name);
		if (field) {
			switch (field->cpp_type()) {
			case FieldDescriptor::CPPTYPE_ENUM:
			{
				// e.g., "en-US" -> "EN_US"
				// try to do a case-insensitive lookup
				to_upper(value);
				boost::replace_all(value, "-", "_");
				const EnumValueDescriptor *enumValue = 0;
				for (int index = 0; index < field->enum_type()->value_count(); ++index) {
					const EnumValueDescriptor *tmp = field->enum_type()->value(index);
					string name(tmp->name());
					to_upper(name);
					boost::replace_all(name, "-", "_");
					if (name==value)
						field->is_repeated()?message->GetReflection()->AddBool(message, field, enumValue):message->GetReflection()->SetEnum(message, field, enumValue);
				}
				break;
			}
			case FieldDescriptor::CPPTYPE_STRING:
			{
				if (field->is_repeated())
					message->GetReflection()->AddString(message, field, value);
				else {
					message->GetReflection()->SetString(message, field, value);
				}
				break;
			}
			default:
				break;
			}
		}
	}
}
