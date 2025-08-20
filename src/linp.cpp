#include <getopt.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <memory>

#include "algo/priorityqueue_lin.h"
#include "algo/queue_lin.h"
#include "algo/set_lin.h"
#include "algo/stack_lin.h"
#include "history_reader.h"

using namespace linp;

typedef std::chrono::steady_clock hr_clock;
// int causes overflow for million operations (e.g. stack algo sums million
// unique values)
typedef long long default_value_type;
const long long defaultEmptyVal = -1;

template <typename value_type>
auto get_monitor(const std::string& type, bool exclude_peeks) {
#define SUPPORT_DS(TYPE)                                       \
  if (type == #TYPE)                                           \
    return exclude_peeks ? TYPE::is_linearizable_x<value_type> \
                         : TYPE::is_linearizable<value_type>;
  SUPPORT_DS(set);
  SUPPORT_DS(stack);
  SUPPORT_DS(queue);
  SUPPORT_DS(priorityqueue);
#undef SUPPORT_DS
  throw std::invalid_argument("Unknown data type");
}

void print_usage() {
  std::cout
      << "Usage: ./linp [-txvh] <history_file>\n"
      << "Options:\n"
      << "  -t\treport time taken in seconds\n"
      << "  -x\texclude peek operations (chooses faster algo if possible)\n"
      << "  -v\tprint verbose information\n"
      << "  -h\tinclude headers\n";
}

int main(int argc, char* argv[]) {
  const char* titles[]{"result", "time_taken", "operations", "exclude_peeks"};
  bool to_print[]{true, false, false, false};
  auto& [_, print_time, print_size, print_xpeeks] = to_print;
  bool print_header = false;
  bool exclude_peeks = false;
  std::string input_file;

  if (argc <= 1) {
    print_usage();
    exit(EXIT_SUCCESS);
  }

  int flag;
  int long_optind;
  static struct option long_options[] = {{"help", no_argument, 0, 0},
                                         {0, 0, 0, 0}};
  while ((flag = getopt_long(argc, argv, "txvh", long_options, &long_optind)) !=
         -1)
    switch (flag) {
      case 0:
        print_usage();
        exit(EXIT_SUCCESS);
      case 't':
        print_time = true;
        break;
      case 'x':
        exclude_peeks = true;
        break;
      case 'v':
        std::fill(to_print, to_print + sizeof(to_print), true);
        break;
      case 'h':
        print_header = true;
        break;
      case '?':
        std::cerr << "Unknown option `" << optopt << "'.\n";
        exit(EXIT_FAILURE);
      default:
        abort();
    }
  if (optind < argc)
    input_file = argv[optind];
  else {
    std::cout << "Please provide a file path\n";
    exit(EXIT_FAILURE);
  }

  history_reader<default_value_type> reader(input_file);
  std::string histType = reader.get_type_s();
  auto monitor = get_monitor<default_value_type>(histType, exclude_peeks);
  auto hist = reader.get_hist();
  size_t operations = hist.size();

  hr_clock::time_point start = hr_clock::now();
  bool result = monitor(hist, defaultEmptyVal);
  hr_clock::time_point end = hr_clock::now();
  long long time_micros =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start)
          .count();

  if (print_header) {
    for (size_t i = 0; i < sizeof(to_print); ++i)
      if (to_print[i]) std::cout << titles[i] << " ";
    std::cout << "\n";
  }

  std::cout << result << " ";
  if (print_time) std::cout << (time_micros / 1e6) << " ";
  if (print_size) std::cout << operations << " ";
  if (print_xpeeks) std::cout << (exclude_peeks ? "true" : "false") << " ";
  std::cout << std::endl;

  return 0;
}