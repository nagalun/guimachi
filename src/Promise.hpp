#pragma once
#include <functional>
#include <variant>

template<typename RR, typename ER>
class Promise {
public:
	enum class State {
		PENDING,
		RESOLVED,
		REJECTED
	};

private:
	std::variant<std::monostate, RR, ER> result;
	std::function<void(RR)> resolveHandler;
	std::function<void(ER)> rejectHandler;
	State state;

public:
	Promise();

	void then(std::function<void(RR)>, std::function<void(ER)>);
	State getState() const;
	void resolve(RR &&);
	void reject(ER &&);
};

#include "Promise.tpp"