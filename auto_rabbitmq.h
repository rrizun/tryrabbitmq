#pragma once

// C includes
#include <stdio.h>
#include <stdlib.h>

// C++ includes
#include <map>
#include <string>
#include <stdexcept>

// lib includes
#include <amqp.h>
#include <amqp_framing.h>

#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>

#include "auto_protobuf_form.h"

using namespace std;
using namespace boost;
//using namespace google::protobuf;

#define THROW(a,...)do{char _throw_tmp[1024];snprintf(_throw_tmp,sizeof(_throw_tmp),"%s:%d "a,__FILE__,__LINE__,##__VA_ARGS__);fprintf(stderr,"%s\n",_throw_tmp);throw runtime_error(_throw_tmp);}while(0)

// local error
static void
die_on_error(int x, char const *context) {
  if (x < 0)
    THROW("%s: %s\n", context, auto_ptr<char>(amqp_error_string(-x)).get());
}

// remote rpc error
static void
die_on_amqp_error(amqp_rpc_reply_t x, char const *context) {
  switch (x.reply_type) {
    case AMQP_RESPONSE_NORMAL:
      return;

    case AMQP_RESPONSE_NONE:
      fprintf(stderr, "%s: missing RPC reply type!\n", context);
      break;

    case AMQP_RESPONSE_LIBRARY_EXCEPTION:
      fprintf(stderr, "%s: %s\n", context, amqp_error_string(x.library_error));
      break;

    case AMQP_RESPONSE_SERVER_EXCEPTION:
      switch (x.reply.id) {
	case AMQP_CONNECTION_CLOSE_METHOD: {
	  amqp_connection_close_t *m = (amqp_connection_close_t *) x.reply.decoded;
	  fprintf(stderr, "%s: server connection error %d, message: %.*s\n",
		  context,
		  m->reply_code,
		  (int) m->reply_text.len, (char *) m->reply_text.bytes);
	  break;
	}
	case AMQP_CHANNEL_CLOSE_METHOD: {
	  amqp_channel_close_t *m = (amqp_channel_close_t *) x.reply.decoded;
	  fprintf(stderr, "%s: server channel error %d, message: %.*s\n",
		  context,
		  m->reply_code,
		  (int) m->reply_text.len, (char *) m->reply_text.bytes);
	  break;
	}
	default:
	  fprintf(stderr, "%s: unknown server error, method id 0x%08X\n", context, x.reply.id);
	  break;
      }
      break;
  }

  throw runtime_error("Oof!");
}

class auto_amqp_bytes {
	amqp_bytes_t bytes;
public:
	auto_amqp_bytes(amqp_bytes_t bytes):bytes(bytes){}
	~auto_amqp_bytes() {amqp_bytes_free(bytes);}
	amqp_bytes_t get() const { return bytes; }
};

struct auto_amqp_connection_state_t {
	amqp_connection_state_t state;
	auto_amqp_connection_state_t(amqp_connection_state_t state) : state(state) {
	}
	~auto_amqp_connection_state_t() {
		amqp_destroy_connection(state);
	}
	amqp_connection_state_t get() const {
		return state;
	}
};

template<class T>
class CmdHandler {
public:
	// dtor
	virtual ~CmdHandler(){}
	// override
	virtual void handleCmd(T *cmd)=0;
};

template<class T>
class EventHandler {
public:
	// dtor
	virtual ~EventHandler(){}
	// override
	virtual void handleEvent(T *event)=0;
};

class auto_rabbitmq {

	class Dispatcher {
	public:
		virtual ~Dispatcher(){}
		virtual void dispatch(Message *raw) = 0;
	};

	template<class T>
	class Dispatcher0: public Dispatcher {
		shared_ptr< EventHandler<T> > handler;
	public:
		// ctor
		Dispatcher0(shared_ptr< EventHandler<T> > handler): handler(handler) {
		}
		virtual void dispatch(Message *raw) {
			T event;
			event.CopyFrom(*raw);
			handler->handleEvent(&event);
		}
	};

	typedef map< string, shared_ptr<Dispatcher> > DispatcherMap;

	string host;
	int port;
	DispatcherMap dispatcherMap;

public:

	auto_rabbitmq(string host, int port): host(host), port(port) {
	}

	// producer api
//	template<class CmdMsg>
//	void postCmd(CmdMsg *cmdMsg) {
//		string name(cmdMsg->GetDescriptor()->name().c_str());
//		printf("postCmd: name=%s\n", name.c_str());
//		dispatchers[name]->dispatch("zzz");
//	}
	template<class EventMsg>
	void event(EventMsg *eventMsg) {
		string name(eventMsg->GetDescriptor()->name().c_str());
		printf("postEventMsg: name=%s\n", name.c_str());

		  auto_amqp_connection_state_t state(amqp_new_connection()); // amqp_destroy_connection

		  amqp_set_sockfd(state.get(), amqp_open_socket(host.c_str(), port));

		  die_on_amqp_error(amqp_login(state.get(), "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "guest", "guest"), "amqp_login");
		  amqp_channel_open(state.get(), 1);
		  die_on_amqp_error(amqp_get_rpc_reply(state.get()), "amqp_channel_open");

		  {
		    amqp_basic_properties_t props;
		    props._flags=0;
//		    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
//		    props.content_type = amqp_cstring_bytes("text/plain");
//		    props.delivery_mode = 2; /* persistent delivery mode */

		    string exchange(name);
		    Form form;
		    ProtoWriteForm(eventMsg, &form);
		    string payload(renderForm(form));
		    string routingkey;

		    die_on_error(amqp_basic_publish(state.get(),
						    1,
						    amqp_cstring_bytes(exchange.c_str()),
						    amqp_cstring_bytes(routingkey.c_str()),
						    0,
						    0,
						    &props,
						    amqp_cstring_bytes(payload.c_str())),
				 "amqp_basic_publish");
		  }

		  die_on_amqp_error(amqp_channel_close(state.get(), 1, AMQP_REPLY_SUCCESS), "Closing channel");
		  die_on_amqp_error(amqp_connection_close(state.get(), AMQP_REPLY_SUCCESS), "Closing connection");
	}

	// consumer api
	template<class EventMsg>
	void eventHandler(shared_ptr< EventHandler<EventMsg> > handler) {
		string name(EventMsg::descriptor()->name().c_str());
		printf("eventHandler: %s\n", EventMsg::descriptor()->name().c_str());
		dispatcherMap[name]=shared_ptr<Dispatcher>(new Dispatcher0<EventMsg>(handler));
	}

	int dispatch() {
		while (1) {
			try {
				auto_amqp_connection_state_t conn(amqp_new_connection());

				int sockfd;
				die_on_error(sockfd = amqp_open_socket(host.c_str(), port), "amqp_open_socket");
				amqp_set_sockfd(conn.get(), sockfd);

				die_on_amqp_error(amqp_login(conn.get(), "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "guest", "guest"), "amqp_login"); // rpc

				amqp_channel_open(conn.get(), 1); // rpc
				die_on_amqp_error(amqp_get_rpc_reply(conn.get()), "amqp_channel_open");

				for (DispatcherMap::iterator iter = dispatcherMap.begin(); iter != dispatcherMap.end(); ++iter) {
					string exchange((*iter).first); // e.g., "OWConfig"

					amqp_exchange_declare(conn.get(), 1, amqp_cstring_bytes(exchange.c_str()), amqp_cstring_bytes("fanout"), 0, 0, amqp_empty_table);
					die_on_amqp_error(amqp_get_rpc_reply(conn.get()), "amqp_exchange_declare");

					amqp_queue_declare_ok_t *r = amqp_queue_declare(conn.get(), 1, amqp_empty_bytes, 0, 0, 0, 1, amqp_empty_table);
					die_on_amqp_error(amqp_get_rpc_reply(conn.get()), "amqp_queue_declare");

					auto_amqp_bytes queuename(amqp_bytes_malloc_dup(r->queue));

					amqp_queue_bind(conn.get(), 1, queuename.get(), amqp_cstring_bytes(exchange.c_str()), amqp_cstring_bytes(""/*bindingkey*/), amqp_empty_table);
					die_on_amqp_error(amqp_get_rpc_reply(conn.get()), "amqp_queue_bind");

					amqp_basic_consume(conn.get(), 1, queuename.get(), amqp_empty_bytes, 0, 1, 0, amqp_empty_table);
					die_on_amqp_error(amqp_get_rpc_reply(conn.get()), "amqp_basic_consume");
				}

				run(conn.get());

				die_on_amqp_error(amqp_channel_close(conn.get(), 1, AMQP_REPLY_SUCCESS), "amqp_channel_close"); // rpc
				die_on_amqp_error(amqp_connection_close(conn.get(), AMQP_REPLY_SUCCESS), "amqp_connection_close"); // rpc

			} catch (std::exception &e) {
				printf("%s\n", e.what());
			}
			time_t now(time(0));
			printf("dispatch[1] %s", ctime(&now));
			sleep(1);
		} // while(1)
		return 0;
	}

	void run(amqp_connection_state_t conn)
	{
	  int received = 0;

	  amqp_frame_t frame;
	  int result;
	  size_t body_received;
	  size_t body_target;

	  while (1) {

	    amqp_maybe_release_buffers(conn);
	    result = amqp_simple_wait_frame(conn, &frame);
	    if (result < 0)
	      return;

	    if (frame.frame_type != AMQP_FRAME_METHOD)
	      continue;

	    if (frame.payload.method.id != AMQP_BASIC_DELIVER_METHOD)
	      continue;

	    amqp_basic_deliver_t *d;
	    d = (amqp_basic_deliver_t *) frame.payload.method.decoded;
	    printf("Delivery %u, exchange %.*s routingkey %.*s\n",
		     (unsigned) d->delivery_tag,
		     (int) d->exchange.len, (char *) d->exchange.bytes,
		     (int) d->routing_key.len, (char *) d->routing_key.bytes);

	    string exchange;
		exchange.assign((const char *) d->exchange.bytes, d->exchange.len);

	    result = amqp_simple_wait_frame(conn, &frame);
	    if (result < 0)
	      return;

	    if (frame.frame_type != AMQP_FRAME_HEADER) {
	      fprintf(stderr, "Expected header!");
	      abort();
	    }

	    body_target = frame.payload.properties.body_size;
	    body_received = 0;

	    string payload;
	    while (body_received < body_target) {
	      result = amqp_simple_wait_frame(conn, &frame);
	      if (result < 0)
		return;

	      if (frame.frame_type != AMQP_FRAME_BODY) {
		fprintf(stderr, "Expected body!");
		abort();
	      }

	      body_received += frame.payload.body_fragment.len;
	      string fragment;
	      fragment.assign((const char *)frame.payload.body_fragment.bytes, frame.payload.body_fragment.len);
	      payload+=fragment;
	      assert(body_received <= body_target);
	    }

	    received++;

    	DispatcherMap::iterator iter = dispatcherMap.find(exchange);
    	if (iter != dispatcherMap.end()) {
    	    const Descriptor *type = DescriptorPool::generated_pool()->FindMessageTypeByName(exchange);
    	    if (type) {
    	    	shared_ptr<Message> message(MessageFactory::generated_factory()->GetPrototype(type)->New());
    	    	ProtoReadForm(message.get(), parseForm(payload)); // interpret msg payload as x-www-form-urlencoded
    		    (*iter).second->dispatch(message.get());
    	    }
    	}
	  }
	}

};
