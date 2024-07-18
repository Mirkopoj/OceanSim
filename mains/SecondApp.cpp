#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

#include "../apps/second_app.hpp"
int main(int argc, char* argv[]) {
	size_t N = 256;
	if (argc > 1) {
		N = std::stoi(argv[1]);
	}
   lve::SecondApp app{N};

   try {
      app.run();
   } catch (const std::exception& e) {
      std::cerr << e.what() << '\n';
      return EXIT_FAILURE;
   }

   return EXIT_SUCCESS;
}
