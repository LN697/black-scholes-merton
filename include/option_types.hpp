#pragma once

namespace bsm {

enum class OptionType { Call, Put };

inline bool is_call(OptionType t) { return t == OptionType::Call; }

}
