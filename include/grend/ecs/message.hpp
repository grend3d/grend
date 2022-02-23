#pragma once

#include <grend/ecs/ecs.hpp>

#include <memory>
#include <vector>
#include <list>

namespace grendx::ecs::messaging {

struct message;
class mailbox;
class endpoint;

struct message {
	std::string type = "undefined";

	entity    *ent  = nullptr;
	component *comp = nullptr;

	// TODO: will this be needed?
	//       could be useful for attaching arbitrary data to be
	//       dynamic_casted<>, but could be a pain with lifetime
	//       management...
	void *data = nullptr;

	// for general info, eg. level... for more storage I guess you'd
	// use the data pointer above, lackluster as it may be
	int tag = 0;
};

// should be far beyond what's needed in practice
static const unsigned maxMessages = 1024;
class mailbox {
	public:
		typedef std::shared_ptr<mailbox> ptr;
		typedef std::weak_ptr<mailbox>   weakptr;

		bool haveMessage(void) { return !messages.empty(); };

		message get(void) {
			if (messages.empty()) { return {}; };

			message ret = messages.front();
			messages.pop_front();
			return ret;
		}

		void add(const message& m) {
			if (messages.size() >= maxMessages) {
				// TODO: warning
				//       (what happened to the TODO about implementing
				//        a proper logger? Need todo that)
				return;
			}

			messages.push_back(m);
		}

		std::list<message> messages;
};

#include <grend/sdlContext.hpp>
class endpoint {
	public:
		typedef std::shared_ptr<endpoint> ptr;
		typedef std::weak_ptr<endpoint>   weakptr;

		void subscribe(mailbox::ptr mbox, std::string type) {
			subscribers[type].push_back(mbox);

			if (debug) {
				SDL_Log("[SUB] %s", type.c_str());
			}
		}

		void unsubscribe(const mailbox *mbox, std::string type) {
			if (subscribers.find(type) == subscribers.end()) {
				return;
			}

			// TODO: not O(N)
			auto& v = subscribers[type];
			for (auto it = v.begin(); it != v.end();) {
				if (auto ptr = it->lock()) {
					it = (ptr.get() == mbox)? v.erase(it) : std::next(it);

				} else {
					it++;
				}
			}
		}

		void unsubscribe(mailbox::ptr mbox, std::string type) {
			unsubscribe(mbox.get(), type);
		}

		void unsubscribe(mailbox::ptr mbox) {
			// TODO
		}

		void publish(const message& message) {
			if (debug) {
				SDL_Log("[PUB] (%s:%s) %s",
						message.ent?  message.ent->typeString()  : "_",
						message.comp? message.comp->typeString() : "_",
						message.type.c_str());
			}

			if (subscribers.find(message.type) == subscribers.end()) {
				// no mailboxes waiting for this message type,
				// nothing to do
				if (debug) {
					SDL_Log("      (No subscribers, dropped)");
				}
				return;
			}

			for (auto& mbox : subscribers[message.type]) {
				if (auto ptr = mbox.lock()) {
				//auto ptr = mbox;
					ptr->add(message);
					if (debug) {
						SDL_Log("      -> sent to subscriber");
					}

				} else {
					if (debug) {
						SDL_Log("      -> couldn't lock subscriber");
					}
					// TODO: remove subscriber from list
				}
			}
		}

		// TODO: toggleable debug levels
		bool debug = false;
		std::map<std::string, std::list<mailbox::weakptr>> subscribers;
		std::map<const mailbox*, std::set<std::string>> subscribedTypes;
};

// namespace grend::ecs::messaging
}
