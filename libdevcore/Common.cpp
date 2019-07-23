#include "Common.h"
#include "Exceptions.h"
#include "Log.h"

#include <brcd/buildinfo.h>
#include <chrono>

using namespace std;
using namespace std::chrono;

namespace dev
{
char const* Version = brcd_get_buildinfo()->project_version;
bytes const NullBytes;
std::string const EmptyString;

void InvariantChecker::checkInvariants(HasInvariants const* _this, char const* _fn, char const* _file, int _line, bool _pre)
{
    if (!_this->invariants())
    {
        cwarn << (_pre ? "Pre" : "Post") << "invariant failed in" << _fn << "at" << _file << ":" << _line;
        ::boost::exception_detail::throw_exception_(FailedInvariant(), _fn, _file, _line);
    }
}

TimerHelper::~TimerHelper()
{
    auto e = std::chrono::high_resolution_clock::now() - m_t;
    if (!m_ms || e > chrono::milliseconds(m_ms))
        clog(VerbosityDebug, "timer")
            << m_id << " " << chrono::duration_cast<chrono::milliseconds>(e).count() << " ms";
}

int64_t utcTime()
{
    // TODO: Fix if possible to not use time(0) and merge only after testing in all platforms
    // time_t t = time(0);
    // return mktime(gmtime(&t));
    return time(0);
}

int64_t utcTimeMilliSec()
{
    //get systime MilliSec
	std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> tp = 
		std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
	auto tmp = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());
	//std::time_t timestamp = tmp.count();
	return  tmp.count();
}

string inUnits(bigint const& _b, strings const& _units)
{
    ostringstream ret;
    u256 b;
    if (_b < 0)
    {
        ret << "-";
        b = (u256)-_b;
    }
    else
        b = (u256)_b;

    u256 biggest = 1;
    for (unsigned i = _units.size() - 1; !!i; --i)
        biggest *= 1000;

    if (b > biggest * 1000)
    {
        ret << (b / biggest) << " " << _units.back();
        return ret.str();
    }
    ret << setprecision(3);

    u256 unit = biggest;
    for (auto it = _units.rbegin(); it != _units.rend(); ++it)
    {
        auto i = *it;
        if (i != _units.front() && b >= unit)
        {
            ret << (double(b / (unit / 1000)) / 1000.0) << " " << i;
            return ret.str();
        }
        else
            unit /= 1000;
    }
    ret << b << " " << _units.front();
    return ret.str();
}

}
