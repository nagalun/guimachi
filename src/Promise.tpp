template<typename RR, typename ER>
Promise<RR, ER>::Promise()
	: state(State::PENDING) { }

template<typename RR, typename ER>
void Promise<RR, ER>::then(std::function<void (RR)> onResolve, std::function<void (ER)> onReject) {
	resolveHandler = std::move(onResolve);
	rejectHandler = std::move(onReject);

	switch (state) {
		case State::REJECTED:
			if (rejectHandler) {
				rejectHandler(std::get<ER>(result));
			}

			break;

		case State::RESOLVED:
			if (resolveHandler) {
				resolveHandler(std::get<RR>(result));
			}

			break;

		default:
			break;
	}
}

template<typename RR, typename ER>
typename Promise<RR, ER>::State Promise<RR, ER>::getState() const {
	return state;
}

template<typename RR, typename ER>
void Promise<RR, ER>::resolve(RR &&resolvedResult) {
	if (state != State::PENDING) {
		return;
	}

	state = State::RESOLVED;

	result = std::forward<RR>(resolvedResult);

	if (resolveHandler) {
		resolveHandler(std::get<RR>(result));
	}
}

template<typename RR, typename ER>
void Promise<RR, ER>::reject(ER &&errorResult) {
	if (state != State::PENDING) {
		return;
	}

	state = State::REJECTED;

	result = std::forward<ER>(errorResult);

	if (rejectHandler) {
		rejectHandler(std::get<ER>(result));
	}
}
