#pragma once

#include <grend/typenames.hpp>
#include <grend/logger.hpp>

#include <memory>
#include <vector>
#include <list>
#include <unordered_map>

namespace grendx::messages {

class mailbox;
class router;

// should be far beyond what's needed in practice
static const unsigned maxMessages = 1024;
class mailbox {
	public:
		typedef std::shared_ptr<mailbox> ptr;
		typedef std::weak_ptr<mailbox>   weakptr;

		bool haveMessage(void) const {
			return !messages.empty();
		};

		// TODO: might be better to return a hash, that way the caller can do
		//       a switch statement with static string hashes...
		const char *frontType() const {
			if (!haveMessage())
				return nullptr;

			return messages.front().first;
		}

		template <typename T>
		bool accept(T& storage) {
			LogErrorFmt("Trying to accept type {}", getTypeName<T>());
			if (!haveMessage())
				return false;

			auto& [name, value] = messages.front();
			// TODO: is it right to assume that the name pointers returned from getTypeName()
			//       will always be the same for the same types?
			//       seems to be the case, but considering my entire ECS system is built
			//       on that it would be good to verify that
			if (getTypeName<T>() == name) {
				void *ptr = value.get();
				T *temp = static_cast<T*>(ptr);
				storage = *temp;
				drop();
				return true;
			}

			return false;
		}

		bool drop() {
			if (haveMessage()) {
				messages.pop_front();
				return true;
			}

			return false;
		}

		template <typename T>
		void add(std::shared_ptr<T> ptr) {
			const char *type = getTypeName<T>();
			LogFmt("queueing message of type {}", type);

			if (messages.size() >= maxMessages) {
				LogErrorFmt("Message dropped, of type {}", type);
				return;
			}

			auto temp = static_pointer_cast<void>(ptr);
			messages.push_back({type, temp});
		}

		std::list<std::pair<const char *, std::shared_ptr<void>>> messages;
};

#include <grend/sdlContext.hpp>
class router {
	public:
		typedef std::shared_ptr<router> ptr;
		typedef std::weak_ptr<router>   weakptr;

		template <typename T>
		void subscribe(mailbox::ptr mbox) {
			const char *type = getTypeName<T>();

			subscribers[type].push_back(mbox);

			if (debug) {
				LogFmt("[SUB] {}", type);
			}
		}

		template <typename T>
		void unsubscribe(const mailbox *mbox) {
			const char *type = getTypeName<T>();

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

		template <typename T>
		void unsubscribe(mailbox::ptr mbox) {
			unsubscribe<T>(mbox.get());
		}

		// TODO: unsubcribe from all
		/*
		void unsubscribe(mailbox::ptr mbox) {
			// TODO
		}
		*/

		// TODO: not a huuuge fan of shared_ptr here, necessarily requiring heap allocation...
		//       might need to redo this, or add some type of object pooling or something
		//       like that, there could potentially be a lot of messages
		//       but, as always, for now it's an alright solution
		template <typename T>
		void publish(std::shared_ptr<T> message) {
			const char *type = getTypeName<T>();

			if (subscribers.find(type) == subscribers.end()) {
				// no mailboxes waiting for this message type,
				// nothing to do
				if (debug) LogFmt("      (No subscribers, dropped {})", type);
				return;
			}

			for (auto& mbox : subscribers[type]) {
				if (auto ptr = mbox.lock()) {
					ptr->add(message);
					if (debug) LogFmt("      -> sent to subscriber");

				} else {
					if (debug) LogFmt("      -> couldn't lock subscriber");
					// TODO: remove subscriber from list
				}
			}
		}

		template <typename T>
		void publish(const T& message) {
			auto ptr = std::make_shared<T>();
			*ptr = message;
			publish<T>(ptr);
		}

		// TODO: toggleable debug levels
		bool debug = true;
		std::unordered_map<const char *, std::vector<mailbox::weakptr>> subscribers;
		std::unordered_map<const mailbox*, std::set<std::string>> subscribedTypes;
};

// namespace grend::ecs::messaging
}
