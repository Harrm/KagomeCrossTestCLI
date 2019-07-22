#define BOOST_ENABLE_ASSERT_HANDLER

#include "kagome/common/hexutil.hpp"
#include "kagome/common/logger.hpp"
#include "kagome/scale/scale.hpp"
#include "kagome/storage/leveldb/leveldb.hpp"
#include "kagome/storage/trie/polkadot_trie_db/polkadot_trie_db.hpp"
#include <boost/program_options.hpp>
#include <iostream>
#include <spdlog/sinks/stdout_sinks.h>

namespace po = boost::program_options;

namespace boost {
void assertion_failed_msg(char const *expr, char const *msg,
                          char const *function, char const *file, long line) {
  std::cerr << file << ":" << line << " " << msg << "\n";
  std::terminate();
}
} // namespace boost

template <typename... Args> class SubcommandRouter {
public:
  void addSubcommand(std::string name, std::function<void(Args...)> handler) {
    handlers[std::move(name)] = std::move(handler);
  }

  std::list<std::string> collectSubcommandNames() const {
    std::list<std::string> names;
    for (auto &[key, value] : handlers) {
      names.push_back(key);
    }
    return names;
  }

  template <typename... FArgs>
  bool executeSubcommand(const std::string &name, FArgs &&... args) {
    if (handlers.find(name) != handlers.end()) {
      handlers.at(name)(std::forward<FArgs>(args)...);
      return true;
    }
    return false;
  }

private:
  std::map<std::string, std::function<void(Args...)>> handlers;
};

void processScaleCodecCommand(int argc, char **argv) {
  po::options_description desc("SCALE codec related tests\nAllowed options:");
  // clang-format off
  desc.add_options()
      ("help", "produce help message")
      ("subcommand", po::value<std::string>(), "specify a subcommand")
      ("input,i", po::value<std::string>(), "the string to be encoded");
  // clang-format on

  po::positional_options_description pd;
  pd.add("subcommand", 1);

  po::variables_map vm;
  po::store(
      po::command_line_parser(argc, argv).options(desc).positional(pd).run(),
      vm);
  po::notify(vm);

  SubcommandRouter<std::string> router;
  router.addSubcommand("encode", [](std::string input) {
    auto res = kagome::scale::encode(input);
    BOOST_ASSERT_MSG(res, "Encode error");
    std::cout << kagome::common::hex_lower(res.value()) << "\n";
  });

  auto subcommand_it = vm.find("subcommand");
  BOOST_ASSERT_MSG(subcommand_it != vm.end(), "Subcommand is not stated");
  auto input_it = vm.find("input");
  BOOST_ASSERT_MSG(input_it != vm.end(), "Input string is not provided");

  BOOST_ASSERT_MSG(
      router.executeSubcommand(subcommand_it->second.as<std::string>(),
                               input_it->second.as<std::string>()),
      "Invalid subcommand");
}

void processStateTrieCommand(int argc, char **argv) {
  po::options_description desc("Trie codec related tests\nAllowed options:");
  // clang-format off
  desc.add_options()
      ("help", "produce help message")
      ("trie-subcommand", po::value<std::string>(), "specify a subcommand")
      ("keys-in-hex", po::value<bool>(), "regard the keys in the trie file as hex values and convert them to binary before inserting into the trie")
      ("state-file,i", po::value<std::string>(), "the file containing the key value data defining the state");
  // clang-format on

  po::positional_options_description pd;
  pd.add("trie-subcommand", 1);

  po::variables_map vm;
  po::store(
      po::command_line_parser(argc, argv).options(desc).positional(pd).run(),
      vm);
  po::notify(vm);

  leveldb::Options opt;
  opt.create_if_missing = true;
  auto logger = spdlog::create<spdlog::sinks::stdout_sink_mt>("DB");
  auto db =
      kagome::storage::LevelDB::create("/tmp/cli_kagome", opt, logger).value();

  BOOST_ASSERT_MSG(db, "DB");
  kagome::storage::trie::PolkadotTrieDb trie(std::move(db));

  SubcommandRouter<std::string> router;
  router.addSubcommand("insert-and-delete", [&trie](std::string input) {
    auto key = kagome::common::unhex(input);
    BOOST_ASSERT_MSG(key, "Key unhex error");
    auto res = trie.get(kagome::common::Buffer{key.value()});
    BOOST_ASSERT_MSG(res, "Trie get error");
    std::cout << kagome::common::hex_lower(res.value()) << "\n";
  });

  auto subcommand_it = vm.find("subcommand");
  BOOST_ASSERT_MSG(subcommand_it != vm.end(), "Subcommand is not stated");
  auto input_it = vm.find("input");
  BOOST_ASSERT_MSG(input_it != vm.end(), "Input string is not provided");

  BOOST_ASSERT_MSG(
      router.executeSubcommand(subcommand_it->second.as<std::string>(),
                               input_it->second.as<std::string>()),
      "Invalid subcommand");
}

int main(int argc, char **argv) {
  SubcommandRouter<int, char **> router;
  router.addSubcommand("scale-codec", processScaleCodecCommand);
  router.addSubcommand("state-trie", processStateTrieCommand);

  std::string commands_list = "Valid subcommands are: ";
  for (auto &&name : router.collectSubcommandNames()) {
    commands_list += name;
    commands_list += " ";
  }
  auto e1 = "Subcommand is not provided\n" + commands_list;
  auto e2 = "Invalid subcommand\n" + commands_list;
  BOOST_ASSERT_MSG(argc > 1, e1.data());
  BOOST_ASSERT_MSG(router.executeSubcommand(argv[1], argc - 1, argv + 1),
                   e2.data());

  return 0;
}
