
#include <STX.hpp>
#include <_hooking.hpp>

#include <regex>

using namespace stx::state;
using namespace stx::mugen;
using namespace stx::hooking;

static inline void rtrim(std::string& s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
		}).base(), s.end());
}

static std::string* parse_raw_string(char* value, char** parseEnd) {

	while (isspace(*value) && *value != '\0') {
		value++;
	}
	if (*value == '\0' || *value == ',') {
		*parseEnd = value;
		return nullptr;
	}

	int c = 0;
	while (*(value + c) != '\0' && *(value + c) != ',') {
		c++;
	}
	*parseEnd = value + c;

	std::string* result = new std::string(value, c);
	rtrim(*result);

	return result;
}

static std::string* parse_quoted_string(char* value, char** parseEnd) {
	while (isspace(*value) && *value != '\0') {
		value++;
	}
	if (*value != '\"') {
		*parseEnd = value;
		return nullptr;
	}
	value++;

	std::string* result = new std::string();

	while (*value != '\0' && *value != '\"') {
		if (*value == '\\') {
			value++;
			if (*value == 'n') {
				*value = '\n';
			}
			if (*value == 't') {
				*value = '\t';
			}
		}
		*result += *value;
		value++;
	}

	if (*value == '\0') {
		*parseEnd = value;
		return nullptr;
	}
	value++;

	while (isspace(*value)) {
		value++;
	}
	*parseEnd = value;

	if (*value != ',' && *value != '\0') {
		return nullptr;
	}

	return result;
}

static arg_t* parse_arg(char** value, PLAYER_INFO* playerInfo, Argument* arg) {
	if (**value == ',') ++*value;
	else if (**value == '\0') return nullptr;

	arg_t* t = new arg_t();
	char* parseEnd = nullptr;

	t->first = arg->get_type();
	switch (t->first) {
	case NUMBER:
	{
		EVAL_VALUE e = {};
		if (SCtrlReadExpList(*value, "n", playerInfo, &parseEnd, &e) != 1) {
			t = nullptr;
			break;
		}
		t->second = e;
		break;
	}
	case INTEGER:
	{
		EVAL_VALUE e = {};
		if (SCtrlReadExpList(*value, "i", playerInfo, &parseEnd, &e) != 1) {
			t = nullptr;
			break;
		}
		t->second = e;
		break;
	}
	case FLOATING_POINT:
	{
		EVAL_VALUE e = {};
		if (SCtrlReadExpList(*value, "f", playerInfo, &parseEnd, &e) != 1) {
			t = nullptr;
			break;
		}
		t->second = e;
		break;
	}
	case QUOTED_STRING:
	{
		std::string* str = parse_quoted_string(*value, &parseEnd);
		if (str == nullptr) {
			t = nullptr;
			break;
		}
		t->second = *str;
		break;
	}
	case RAW_STRING:
	{
		std::string* str = parse_raw_string(*value, &parseEnd);
		if (str == nullptr) {
			t = nullptr;
			break;
		}
		t->second = *str;
		break;
	}
	default:
	{
		return nullptr;
	}
	}

	*value = parseEnd;
	return t;
}

static arg_t* set_const_arg(PLAYER_INFO* playerInfo, Argument* arg) {
	arg_t* t = new arg_t();

	t->first = arg->get_type();
	switch (t->first) {
	case NUMBER:
	{
		auto p = dynamic_cast<Argument_Optional<number>*>(arg);
		auto v = p->get_value();

		EVAL_VALUE e = {};
		if (std::holds_alternative<int>(v)) {
			ConstExpI(&e, std::get<int>(v));
		}
		else {
			ConstExpF(&e, std::get<float>(v));
		}
		t->second = e;

		break;
	}
	case INTEGER:
	{
		auto p = dynamic_cast<Argument_Optional<int>*>(arg);
		int v = std::get<int>(p->get_value());

		EVAL_VALUE e = {};
		ConstExpI(&e, v);
		t->second = e;
		break;
	}
	case FLOATING_POINT:
	{
		auto p = dynamic_cast<Argument_Optional<float>*>(arg);
		float v = std::get<float>(p->get_value());

		EVAL_VALUE e = {};
		ConstExpF(&e, v);
		t->second = e;
		break;
	}
	case QUOTED_STRING:
	{
		auto p = dynamic_cast<Argument_Optional<QuotedString>*>(arg);
		std::string v = std::get<std::string>(p->get_value());

		std::string* str = new std::string(v);
		t->second = *str;
		break;
	}
	case RAW_STRING:
	{
		auto p = dynamic_cast<Argument_Optional<RawString>*>(arg);
		std::string v = std::get<std::string>(p->get_value());

		std::string* str = new std::string(v);
		t->second = *str;
		break;
	}
	default:
	{
		return nullptr;
	}
	}

	return t;
}

static param_t* parse_param(char** value, PLAYER_INFO* playerInfo, std::vector<std::pair<std::string, Argument*>> args_req, std::vector<std::pair<std::string, Argument*>> args_opt) {
	param_t* param = new param_t(new args_t(), new args_t());
	// 必須引数のパース
	for (auto& arg : args_req) {
		arg_t* a = parse_arg(value, playerInfo, arg.second);
		if (a == nullptr) {
			return nullptr;
		}
		param->first->insert({ arg.first, a });
	}

	// 任意引数のパース
	for (auto& arg : args_opt) {
		arg_t* a = nullptr;
		if (*(*value) != '\0') {
			a = parse_arg(value, playerInfo, arg.second);
		}
		else {
			a = set_const_arg(playerInfo, arg.second);
		}
		if (a == nullptr) {
			return nullptr;
		}
		param->second->insert({ arg.first, a });
	}

	return param;
}

BOOL stx::hooking::regmodstate(int32_t original_result, TPFILE* tpf, SCX_DATA_EX* sinfo, PLAYER_INFO* playerInfo) {
	if (original_result == TRUE) return original_result;

	const char* mugen_error = (const char*)*((DWORD*)0x4b5b4c) + 0xC534;
	std::regex pattern("Not a valid elem type: (.*)");
	std::cmatch match;

	if (!std::regex_search(mugen_error, match, pattern)) {
		return original_result;
	}

	if (match.size() < 2) {
		return original_result;
	}

	controller::Controller* ctrl = controller::Controller::get_ctrl(match.str(1));
	if (!ctrl) {
		return original_result;
	}

	params_t* ps_t = new params_t();
	// 必須パラメータの処理
	auto params_req = ctrl->get_params_req();
	for (auto param : params_req) {
		auto p = dynamic_cast<Parameter_Required*>(param.second);
		auto p_name = p->get_name();

		char* value = TPGetValue(tpf, p_name.c_str());
		if (!value) {
			SetParseErrorText(replace(p->get_missing_error(), ctrl->get_name(), p_name).c_str());
			return original_result;
		}
		// パラメータを処理
		auto param = parse_param(&value, playerInfo, p->get_args_req(), p->get_args_opt());
		if (param == nullptr) {
			SetParseErrorText(replace(p->get_illegal_error(), ctrl->get_name(), p_name).c_str());
			return original_result;
		}
		ps_t->insert({ p_name, param });
	}

	sinfo->scID = 0x7FFFFFFF;
	sinfo->proc = new processor::Processor(ps_t, ctrl->get_proc());
	return TRUE;
}