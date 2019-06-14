// Contains the list of all arguments
#include <initializer_list>
#include <algorithm>
#include <iterator>
#include <boost/bind.hpp>
#include <boost/program_options.hpp>
#include <boost/program_options/errors.hpp>

namespace po = boost::program_options;

namespace soplex
{

  // Runs SoPlex with parsed boost variables map
  template <class R>
  int runSoPlex(const po::variables_map& vm);


  // Checks if value is between [min, max] and if not throws an exception
  template <typename T>
  void checkRange(const T& val, const T& min, const T& max)
  {
    if(val < min || val > max)
      {
        // TODO Maybe I should write the name?
        // throw po::validation_error::invalid_option_value;
        // Figure this out
      }
  }

  // Checks if the element val is in the initializer_list
  template <typename T>
  void in(const T& val, std::initializer_list<T> list)
  {
    auto lEnd = list.cend();
    auto iter = std::find(val, list.cbegin(), lEnd);

    if(iter == lEnd)            // meaning that val is not in the list
      {
        throw po::validation_error::invalid_option_value;
      }
  }


  // Parses the command line arguments
  auto parseArgs(int argc, char* argv[]) -> int
  {

    int solvemode = 1;


    po::positional_options_description p;
    p.add("lpfile", -1);

    // Define all the options
    po::options_description generic("generic options");
    generic.add_options()
      ("help,h", "help")
      ("version", "version");

    // Problem: Usually the descriptors should be just readbas. But how do I
    // print something like readbas=<basfile>?
    po::options_description general("general options");
    general.add_options()
      ("lpfile", po::value<std::string>(), "the lp file")
      ("readbas", po::value<std::string>(),  "read starting basis from file")
      ("writebas", po::value<std::string>(), "write terminal basis to file")
      ("writefile", po::value<std::string>(), "write LP to file in LP or MPS format depending on extension")
      ("writedual", po::value<std::string>(),  "write the dual LP to a file in LP or MPS formal depending on extension")
      ("<type>:<name>=<val>", "change parameter value using syntax of settings file entries") // TODO: How do I deal with this?
      ("loadset", po::value<std::string>(), "load parameters from settings file (overruled by command line parameters") // TODO: How do I deal with overruled?
      ("saveset", po::value<std::string>(), "save parameters to settings file")
      ("diffset", po::value<std::string>(), "save modified parameters to settings file")
      ("extsol", po::value<std::string>(), "external solution for soplex to use for validation");

    po::options_description lt("limits and tolerances");
    lt.add_options()
      ("time,t", po::value<int>(), "set time limit to n seconds")
      ("iterlimit,i", po::value<int>(), "set iteration limit to n")
      ("primfeastol,f", po::value<double>(), "set primal feasibility tolerance to double") // fix
      ("dualfeastol,o", po::value<double>(),"set dual feasibility (optimality) tolerance to") // vix
      ("valtol,l", po::value<double>(), "set validation tolerance to whatever"); // fix

    // Variables will contain value specified by user or the default

    po::options_description algo("algorithmic settings (default in brackets)");
    algo.add_options()
      ("readmode", po::value<int>()->default_value(0)->notifier(boost::bind(&checkRange<int>, _1, 0, 1)),
       "choose reading mode for <lpfile> (0 - floating-point, 1 - rational)")
      ("solvemode", po::value<int>(&solvemode)->default_value(1)->notifier(boost::bind(&checkRange<int>, _1, 0, 3)),
       "choose solving mode (0 - floating-point solve, 1 - auto, 2 - force iterative refinement, 3 - multi precision solve)")
      ("simplifier,s", po::value<int>()->default_value(1)->notifier(boost::bind(&checkRange<int>, _1, 0, 3)),
       "choose simplifier/presolver (0 - off, 1 - auto)")
      ("scaler,g", po::value<int>()->default_value(2)->notifier(boost::bind(&checkRange<int>, _1, 0, 6)),
       "choose scaling (0 - off, 1 - uni-equilibrium, 2 - bi-equilibrium, 3 - geometric, 4 - iterated geometric, 5 - least squares, 6 - geometric-equilibrium)")
      ("pricer,p", po::value<int>()->default_value(0)->notifier(boost::bind(&checkRange<int>, _1, 0, 5)),
       "choose pricing (0 - auto, 1 - dantzig, 2 - parmult, 3 - devex, 4 - quicksteep, 5 - steep)")
      ("ratiotester,r", po::value<int>()->default_value(3)->notifier(boost::bind(&checkRange<int>, _1, 0, 3)),
       "choose ratio tester (0 - textbook, 1 - harris, 2 - fast, 3 - boundflipping)");

    po::options_description display("display options");
    display.add_options()
      ("verbosity,v", po::value<int>()->default_value(3)->notifier(boost::bind(&checkRange<int>, _1, 0, 5)), // TODO: Figure this out
       "set verbosity to <level> (0 - error, 3 - normal, 5 - high)") // fix the default
      ("printprimal,x",  "print primal solution")
      ("printdualmult,y", "print dual multipliers")
      ("printratsol,X", "print primal solution in rational numbers")
      ("printdualmultrational,Y", "print dual multipliers in rational numbers")
      ("dispstat,q", "display detailed statistics")
      ("checkfinal,c", "perform final check of optimal solution in original problem");

    // These are parameters corresponding to the boolParam in SoPlex<R>::Settings
    // class. Ideally I want the parsing of arguments to finish before creating a
    // SoPlex Object. In the old SoPlex, half of the parsing happened outside
    // SoPlex and the rest inside the SoPlex.
    po::options_description boolParam("bool Parameters");
    boolParam.add_options()
      ("bool:lifting", po::value<bool>()->default_value(false), "should lifting be used to reduce range of nonzero matrix coefficients?")
      ("bool:eqtrans", po::value<bool>()->default_value(false), "should LP be transformed to equality form before a rational solve?")
      ("bool:testdualinf", po::value<bool>()->default_value(false), "should dual infeasibility be tested in order to try to return a dual solution even if primal infeasible?");

    po::options_description mpf("Multiprecision float solve");
    mpf.add_options()
      ("mpf", "Run templated multi-precision SoPlex") // This is redundant; there is the solvemode param
      ("precision", po::value<unsigned int>()->default_value(100), "Minimum precision of mpf float");

    po::options_description allOpt("Allowed options");
    allOpt.add(generic).add(general).add(lt).add(algo).add(display);

    try
      {
        // A hash map that takes a string to a boost::any type. The boost:any
        // type can be casted into the value we need.
        po::variables_map vm;
        // TODO: Maybe replace the parse_command_line with the other function?
        po::store(po::command_line_parser(argc, argv).options(allOpt).positional(p).run(), vm);


        // If help is an option, print the help message and quit
        if(vm.count("help"))
          {
            std::cout<<allOpt<<"\n";
            return 0;
          }

        // Relevant: https://stackoverflow.com/a/5517755/4223038 This will make
        // sure that all the required options are given, If not boost should
        // (throw?) print an appropriate error. Also the notifier function while
        // making the argument makes sure that a condition is met, such as
        // whether the value given is in a certain range or inside an
        // initializer_list
        po::notify(vm);

        switch(solvemode)
          {
          case 0:                 // floating point
          case 1:                 // auto
          case 2:                 // iterative refinement
            runSoPlex<Real>(vm);
            break;
          case 3:                 // soplex mpf
            std::cout<<"You are now running on mpf\n"; // TODO: mpf
            break;
          default:
            std::cout<<"Wrong value for the solve mode\n\n"<<allOpt<<"\n";
            return 0;
          }

      }
    catch(std::exception& e)
      {
        // TODO: How does SoPlex Handle the std::cerr?
        std::cerr<<"error: "<<e.what()<<"\n\n";
        // print the help message
        std::cout<<allOpt<<"\n";
        return 1;
      }
    catch(...)
      {
        std::cerr<<"Exception of unknown type\n";
      }

    return 0;

  }



}
