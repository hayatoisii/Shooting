#pragma once

#include <KamataEngine.h>
#include <vector>

// Observer Pattern: 衝突応答・スコア加算・爆発演出を疎結合にする
enum class GameEventType {
	EnemyDestroyed,
	ScoreChanged,
	ExplosionRequested,
};

struct GameEvent {
	GameEventType type = GameEventType::EnemyDestroyed;
	KamataEngine::Vector3 position = {0.0f, 0.0f, 0.0f};
	int scoreDelta = 0;
	int totalScore = 0;
};

class IGameEventListener {
public:
	virtual ~IGameEventListener() = default;
	virtual void OnGameEvent(const GameEvent& event) = 0;
};

class GameEventSubject {
public:
	void Subscribe(IGameEventListener* listener);
	void Unsubscribe(IGameEventListener* listener);
	void Notify(const GameEvent& event);

private:
	std::vector<IGameEventListener*> listeners_;
};
