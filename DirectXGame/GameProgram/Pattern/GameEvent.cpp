#include "GameEvent.h"

#include <algorithm>

void GameEventSubject::Subscribe(IGameEventListener* listener) {
	if (listener == nullptr) {
		return;
	}
	if (std::find(listeners_.begin(), listeners_.end(), listener) == listeners_.end()) {
		listeners_.push_back(listener);
	}
}

void GameEventSubject::Unsubscribe(IGameEventListener* listener) {
	listeners_.erase(std::remove(listeners_.begin(), listeners_.end(), listener), listeners_.end());
}

void GameEventSubject::Notify(const GameEvent& event) {
	for (IGameEventListener* listener : listeners_) {
		if (listener != nullptr) {
			listener->OnGameEvent(event);
		}
	}
}
