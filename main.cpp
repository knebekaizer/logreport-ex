
#include "IpSummary.h"

#include <exception>

#ifndef UT_CATCH
int main (int argc, const char * argv[])
{
	try {
		IpSummary ips(argc, argv);
		auto err = ips.run();
		return err;
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
		return -1;
	}
}
#endif // CATCH_CONFIG_MAIN

