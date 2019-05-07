
#include "IpSummary.h"

#include <exception>



#ifndef UT_CATCH


static std::vector<std::string> argsParser(int argc, const char * argv[])
{
	using namespace std;
	std::vector<std::string> args;
	if (argc > 1 && argc < 5) {
		int k = 1;
		for ( ; k < argc; ++k) {
			args.emplace_back(argv[k]);
		}
		for ( ; k < 4; ++k) {
			args.emplace_back("-");
		}
	} else {
		cout << "Usage: " << argv[0] << " <input_1> <input_2> <output>" << endl;
		cout <<
		     "Parameters are file names or \"-\" for standart input or output,"
		     "  last two may be omitted.\n"
		     "  input_1: Customers database\n"
		     "  input_2: IP log\n"
		     "  output:  Traffic summary by customer ID\n"
		     << endl;
	}
	return args;
}

int main (int argc, const char * argv[])
{
	try {
		auto args = argsParser(argc, argv);
		if (args.empty())
			return -1;

		IpSummary ips;
		int err = ips.run(args);
		return err;
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
		return -1;
	}
}
#endif // CATCH_CONFIG_MAIN

