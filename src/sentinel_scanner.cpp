#include <string>
#include "core/sentinel_scanner.h"
#include <utility>

SentinelScanner::SentinelScanner(std::string sentinel)
	: sentinel_(sentinel), pending_() {
}

SentinelScanner::Out SentinelScanner::feed(std::string_view chunk) {
	pending_.append(chunk);
	size_t pos = pending_.find(sentinel_);
	if (pos != std::string::npos) {
		std::string safe_text = pending_.substr(0, pos);
		pending_.clear();
		return { safe_text, true };
	}
	else {
		size_t safe_length = (pending_.size() >= sentinel_.size()) ? pending_.size() - sentinel_.size() + 1 : 0;
		std::string safe_text = pending_.substr(0, safe_length);
		pending_ = pending_.substr(safe_length); 
		return { safe_text, false };
	}
}
SentinelScanner::Out SentinelScanner::flush() {
	std::string safe_text = std::move(pending_);
	pending_.clear();
	return { safe_text, false };
}