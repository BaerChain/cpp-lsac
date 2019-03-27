#include "Base64.h"

using namespace std;
using namespace dev;

static inline bool is_base64(byte c)
{
	return (isalnum(c) || (c == '+') || (c == '/'));
}

static inline byte find_base64_char_index(byte c)
{
	if ('A' <= c && c <= 'Z') return c - 'A';
	else if ('a' <= c && c <= 'z') return c - 'a' + 1 + find_base64_char_index('Z');
	else if ('0' <= c && c <= '9') return c - '0' + 1 + find_base64_char_index('z');
	else if (c == '+') return 1 + find_base64_char_index('9');
	else if (c == '/') return 1 + find_base64_char_index('+');
	else return 1 + find_base64_char_index('/');
}

string dev::toBase64(bytesConstRef _in)
{
	static const char base64_chars[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/";

	string ret;
	int i = 0;
	int j = 0;
	byte char_array_3[3];
	byte char_array_4[4];

	auto buf = _in.data();
	auto bufLen = _in.size();

	while (bufLen--)
	{
		char_array_3[i++] = *(buf++);
		if (i == 3)
		{
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for (i = 0; i < 4; i++)
				ret += base64_chars[char_array_4[i]];
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j < 3; j++)
			char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (j = 0; j < i + 1; j++)
			ret += base64_chars[char_array_4[j]];

		while (i++ < 3)
			ret += '=';
	}

	return ret;
}

bytes dev::fromBase64(string const& encoded_string)
{
	auto in_len = encoded_string.size();
	int i = 0;
	int j = 0;
	int in_ = 0;
	byte char_array_3[3];
	byte char_array_4[4];
	bytes ret;

	while (in_len-- && encoded_string[in_] != '=' && is_base64(encoded_string[in_]))
	{
		char_array_4[i++] = encoded_string[in_]; in_++;
		if (i == 4)
		{
			for (i = 0; i < 4; i++)
				char_array_4[i] = find_base64_char_index(char_array_4[i]);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
				ret.push_back(char_array_3[i]);
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j < 4; j++)
			char_array_4[j] = 0;

		for (j = 0; j < 4; j++)
		char_array_4[j] = find_base64_char_index(char_array_4[j]);

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; j < i - 1; j++)
			ret.push_back(char_array_3[j]);
	}

	return ret;
}
