/*
 * TestDataFixtureBase.hpp
 *
 *  Created on: May 27, 2015
 *      Author: fbeier
 */

#ifndef LIBCPP_TEST_UTILITIES_TESTDATAFIXTUREBASE_HPP_
#define LIBCPP_TEST_UTILITIES_TESTDATAFIXTUREBASE_HPP_

#include "TestFixtureException.hpp"
#include "test_utilities_config.hpp"
#include "libcpp/utilities/EnvironmentVariable.hpp"
#include "libcpp/types/types.hpp"
#include "libcpp/preprocessor/MakeString.hpp"

#include <list>
#include <string>
#include <cassert>
#include <boost/optional.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>


namespace ns_test_utilities {

/**
 * @brief Base class for a unit test fixture that grants access to a common test data directory.
 *
 * This utility class is intended to be used for unit tests via the boost unit test framework.
 * It can be used as fixture class to inject common setup and teardown routines.
 *
 * This fixture base class grants access to test data files which can be put under version control.
 * It assumes that all test data files are stored under a common test data root file system
 * directory and might be grouped in nested sub-directories for different test modules.
 * The path to the (existing) root directory for test data must be provided by the user either:
 *   1. by specifying a @c TEST_DATA_ROOT environment variable containing the path to the directory
 *      to be used
 *   2. providing the default path to be used in a @c test_utilities_config.hpp header file
 *      that needs to be in the include search path when the test is compiled. A template for
 *      this file is provided together with this header and can easily configured using CMake
 *      to inject the path for the build tree.
 *   .
 *
 * Test (sub)modules can be declared with inheriting from this base class and can define common
 * setup steps within their constructors or and teardown steps in destructors repectively.
 * Further, the can specify their own path with overriding the @c getTestModuleDataDirectory()
 * method. Ususally, this returns a directory under the data directory of their base module.
 * For this purpose, the @c TEST_MODULE_DATA_DIRECTORY can be used to generate the boilerplate code.
 *
 * This default data directory for the test modules can be explicitly overwritten by the user
 * with specifying setting a TEST_DATA_DIR environment variable to the path of the directory
 * which shall be used instead. Note, that this overrides the directory at runtime for all tests
 * that are executed within the same (shell) process and use a fixture inheriting from this base class.
 *
 * @see http://www.boost.org/doc/libs/1_58_0/libs/test/doc/html/utf/user-guide/fixture.html
 *
 * @author Felix Beier <felix.beier@tu-ilmenau.de>
 */
class TestDataFixtureBase {
private:

	/// a mutex for protecting access to generated output data files
	typedef boost::mutex Mutex;

	/// a scoped lock for the mutex
	typedef boost::lock_guard< Mutex > Lock;


	/// name of the sub-directory the test uses for storing its output
	static constexpr const char* DEFAULT_TEST_OUTPUT_DIR_NAME = "generated";


public:

	/// an operating system path to a file/directory
	typedef boost::filesystem::path Path;

	/// an input file stream for reading existing test data
	typedef boost::filesystem::ifstream InputDataFile;

	/// pointer to an input file stream for reading existing test data
	typedef ns_types::SharedPtr< InputDataFile > InputDataFilePtr;

	/// an output file stream for writing generated test data
	typedef boost::filesystem::ofstream OutputDataFile;

	/// pointer to an output file stream for writing generated test data
	typedef ns_types::SharedPtr< OutputDataFile > OutputDataFilePtr;


	/**
	 * @brief Common @c setup routine.
	 *
	 * This constructor initializes the fixture and sets the path to the root directory
	 * that comprises all data for the test.
	 *
	 * @throws TestFixtureException
	 *    if the no valid data root directory path can be determined
	 */
	TestDataFixtureBase() {
		mTestDataRoot = getTestDataRootDirectoryPath();
	}

	/**
	 * @brief Common @c teardown routine.
	 *
	 * Nothing needs to be done here.
	 *
	 * @note We do not cleanup generated test data for two reasons:
	 *         1. It might be required and examined after the test completes.
	 *         2. The user is able to customize the directory path that shall be used
	 *            to store test output to an existing directory. We don't want to delete
	 *            anything there in case it points to important stuff by accident.
	 *            We leave this to the user to cleanup the environment before.
	 */
	virtual ~TestDataFixtureBase() {}


	/**
	 * @brief Get the path to the root directory storing all test data files.
	 *
	 * @return an operating system path to the test data root directory.
	 */
	Path getTestDataRoot() const {
		return mTestDataRoot;
	}

	/**
	 * @brief Get a path to the directory that comprises data for the test module.
	 *
	 * This method returns the path that is used to store data for the test module.
	 * The test data directory is determined in the following way:
	 *   1. If the @c TEST_DATA_DIR environment variable is set and points to an existing
	 *      directory in the file system, its path will be used.
	 *   2. If the @c TEST_DATA_DIR environment variable is not set, or does not point to
	 *      a valid directory the default directory will be used. The path to the default
	 *      directory will be set by the actual test fixture module inheriting from this
	 *      base class with overriding the @c getTestModuleDataDirectory() method
	 *      which usually points to a (possibly nested) sub-directory of the root directory.
	 *   .
	 *
	 * @throws TestFixtureException
	 *    if the no valid data directory path can be determined
	 * @return a file system path to the test data directory
	 */
	Path getTestDataDirectory() const {
		using ns_utilities::exceptions::error_info::Description;
		namespace fs = boost::filesystem;

		std::list< std::string > testDataDirs;

		// 1. try to read the TEST_DATA_DIR environment variable which can be used to overwrite defaults
		ns_utilities::EnvironmentVariable TEST_DATA_DIR = getEnvironmentVariable( "TEST_DATA_DIR" );
		if( TEST_DATA_DIR.is_initialized() ) {
			testDataDirs.push_back( TEST_DATA_DIR.get() );
		}

		// 2. default path for the actual test module relative to the root directory
		const std::string moduleDataDir = getTestModuleDataDirectory().string();
		testDataDirs.push_back( moduleDataDir );

		boost::optional< Path > testDataDir = getDirectoryPath( testDataDirs );
		if( !testDataDir ) {
			BOOST_THROW_EXCEPTION( exceptions::TestFixtureException()
				<< Description( "Invalid directory for test data!" )
				<< Description( MAKE_STRING( "environment variable " << TEST_DATA_DIR.getName() << ": "
						<< ( TEST_DATA_DIR.is_initialized() ?
							TEST_DATA_DIR.get() : std::string( "NOT SET" )
						)
					))
				<< Description( MAKE_STRING( "default test module data directory: "
						<< moduleDataDir
					))
			);
		}

		assert( fs::is_directory( *testDataDir ) );
		return *testDataDir;
	}

	/**
	 * @brief Get a path to the directory that shall be used for storing generated test output.
	 *
	 * This method returns the path that the test module can use for storing generated output data.
	 * The output directory is determined in the following way:
	 *   1. If the @c TEST_OUTPUT_DIR environment variable is set, this directory will be used.
	 *      If it does not point to a valid directory, a @c TestFixtureException is thrown.
	 *   2. If the @c TEST_OUTPUT_DIR environment variable is not set, the default directory will be used.
	 *      The default directory with name @c DEFAULT_TEST_OUTPUT_DIR_NAME will be stored relative
	 *      to the test data directory that can be obtained by @c getTestDataDirectory().
	 *      If it does not exist, it will be created. If it exists but does not point to a
	 *      directory, a @c TestFixtureException is thrown.
	 *   .
	 *
	 * @throws TestFixtureException
	 *    if the test output directory path does not point to a valid location
	 * @return a file system path to the test output directory
	 */
	Path getTestOutputDirectory() const {
		using ns_utilities::exceptions::error_info::Description;
		namespace fs = boost::filesystem;

		assert( fs::is_directory( mTestDataRoot ) );
		Path testOutputDir;

		// 1. try to read the TEST_OUTPUT_DIR environment variable
		ns_utilities::EnvironmentVariable TEST_OUTPUT_DIR = getEnvironmentVariable( "TEST_OUTPUT_DIR" );
		if( TEST_OUTPUT_DIR.is_initialized() ) {
			// the variable is set -> it must point to a valid directory
			testOutputDir = TEST_OUTPUT_DIR.get();
			if( !fs::is_directory( testOutputDir ) ) {
				BOOST_THROW_EXCEPTION( exceptions::TestFixtureException()
					<< Description( "Invalid directory for test output data!" )
					<< Description( MAKE_STRING( "environment variable " << TEST_OUTPUT_DIR.getName() << ": "
							<< TEST_OUTPUT_DIR.get()
						))
				);
			}
		}
		else {
			// prevent other threads to access the output directory through this class
			Lock lock( gMutex );

			// 2. default path relative to the directory used for the test module
			testOutputDir = getTestDataDirectory();
			testOutputDir /= DEFAULT_TEST_OUTPUT_DIR_NAME;

			if( fs::exists( testOutputDir ) && !fs::is_directory( testOutputDir ) ) {
				BOOST_THROW_EXCEPTION( exceptions::TestFixtureException()
					<< Description( "Invalid default directory for test output data!" )
					<< Description( MAKE_STRING( "path to directory " << testOutputDir
						<< " does already exist but is not a directory"
						))
				);
			}

			// create if if it doesn't exist
			if( !fs::exists( testOutputDir ) ) {
				boost::system::error_code error;
				bool created = fs::create_directory( testOutputDir, error );
				if( !created ) {
					BOOST_THROW_EXCEPTION( exceptions::TestFixtureException()
						<< Description( "Unable to create default directory for test output data!" )
						<< Description( MAKE_STRING( "path to directory " << testOutputDir ))
						<< Description( MAKE_STRING( "error code: " << error ))
					);
				}
			}
		}

		assert( fs::is_directory( testOutputDir ) );
		return testOutputDir;
	}


	/**
	 * @brief Check if a specific file can be found under the directory that stores the
	 *        test data for the module fixture.
	 *
	 * The requested file will be searched relative to the module's test data directory
	 * that will be returned via @c getTestDataDirectory().
	 *
	 * @param[in] fileName
	 *    the name of the requested file
	 * @return @c true if a regular file can be found within the module's test data directory
	 *         @c false otherwise
	 */
	bool testFileExists( const std::string& fileName ) const {
		namespace fs = boost::filesystem;

		Path testFilePath = getTestDataDirectory();
		testFilePath /= fileName;

		return fs::is_regular_file( testFilePath );
	}

	/**
	 * @brief Get the operating system path to a test data file.
	 *
	 * The requested file will be searched relative to the module's test data directory
	 * that will be returned via @c getTestDataDirectory().
	 *
	 * @throws @c TestFixtureException
	 *    if the requested test data file could not be found
	 * @param[in] fileName
	 *    the name of the requested file
	 * @return an operating system file path to the requested test file
	 */
	Path getTestFileName( const std::string& fileName ) const {
		using ns_utilities::exceptions::error_info::Description;
		namespace fs = boost::filesystem;

		Path testFilePath = getTestDataDirectory();
		testFilePath /= fileName;

		if( !testFileExists( fileName ) ) {
			ns_utilities::EnvironmentVariable TEST_DATA_DIR = getEnvironmentVariable( "TEST_DATA_DIR" );

			BOOST_THROW_EXCEPTION( exceptions::TestFixtureException()
				<< Description( "Unable to find requested test file!" )
				<< Description( MAKE_STRING( "requested file name: " << fileName ))
				<< Description( MAKE_STRING( "resolved file path: " << testFilePath ))
				<< Description( MAKE_STRING( "environment variable " << TEST_DATA_DIR.getName() << ": "
						<< ( TEST_DATA_DIR.is_initialized() ?
							TEST_DATA_DIR.get() : std::string( "NOT SET" )
						)
					))
			);
		}
		
		return testFilePath;
	}
	
	/**
	 * @brief Get an input file stream for reading a test data file.
	 *
	 * The requested file will be searched relative to the module's test data directory
	 * that will be returned via @c getTestDataDirectory().
	 *
	 * @throws @c TestFixtureException
	 *    if the requested test data file could not be found
	 * @param[in] fileName
	 *    the name of the requested file
	 * @return an input file stream for reading the file
	 */
	InputDataFilePtr getTestFile( const std::string& fileName ) const {
		using ns_utilities::exceptions::error_info::Description;

		Path testFilePath = getTestFileName( fileName );
		return ns_types::make_shared< InputDataFile >( testFilePath );
	}

	/**
	 * @brief Check if a specific file can be found under the directory that stores the
	 *        test output data for the module fixture.
	 *
	 * The requested file will be searched relative to the module's test output data directory
	 * that will be returned via @c getTestOutputDirectory().
	 *
	 * @param[in] fileName
	 *    the name of the requested file
	 * @return @c true if a regular file can be found within the module's test output data directory
	 *         @c false otherwise
	 */
	bool testOutputFileExists( const std::string& fileName ) const {
		namespace fs = boost::filesystem;

		Path testFilePath = getTestOutputDirectory();
		testFilePath /= fileName;

		return fs::is_regular_file( testFilePath );
	}

	/**
	 * @brief Get an input file stream for reading a test data file.
	 *
	 * The requested file will be searched relative to the module's test data directory
	 * that will be returned via @c getTestOutputDirectory().
	 *
	 * @throws @c TestFixtureException
	 *    if the requested test data file could not be found
	 * @param[in] fileName
	 *    the name of the requested file
	 * @return an input file stream for reading the file
	 */
	OutputDataFilePtr createTestOutputFile( const std::string& fileName ) const {
		using ns_utilities::exceptions::error_info::Description;

		Path testFilePath = getTestOutputDirectory();
		testFilePath /= fileName;

		if( testOutputFileExists( fileName ) ) {
			BOOST_THROW_EXCEPTION( exceptions::TestFixtureException()
				<< Description( "Test output file to be created already exist!" )
				<< Description( MAKE_STRING( "requested file name: " << fileName ))
				<< Description( MAKE_STRING( "resolved file path: " << testFilePath ))
			);
		}

		return ns_types::make_shared< OutputDataFile >( testFilePath );
	}


protected:

	/**
	 * @brief
	 *
	 * This interface method needs to be overridden by each test module fixture to provide
	 * a path to the directory that shall be used to store the module's data.
	 * Usually, a (possibly nested) sub-directory of this class will be used.
	 *
	 * @note The @c TEST_MODULE_DATA_DIRECTORY macro can be used to define a sub-directory
	 *       relative to the parent module.
	 *
	 * @return a file system path pointing to the directory where test data shall be stored
	 */
	virtual Path getTestModuleDataDirectory() const {
		return getTestDataRoot();
	}


private:

	/**
	 * @brief Helper method which determines the root directory used for storing all test data.
	 *
	 * This method is used in the constructor and determines the file system path pointing
	 * to an existing directory that shall be used to store all test data.
	 *
	 * The data root directory is determined in the following way:
	 *   1. If the @c TEST_DATA_ROOT environment variable is set and points to an existing
	 *      directory in the file system, its path will be used.
	 *   2. If the @c TEST_DATA_ROOT environment variable is not set, or does not point to
	 *      a valid directory the default directory will be used. The path to the default
	 *      directory will be read from the @c test_utilities_config.hpp generated header file.
	 *   .
	 *
	 * @throws TestFixtureException
	 *    if the no valid data root directory path can be determined
	 * @return a file system path pointing to the root directory for storing test data
	 */
	static Path getTestDataRootDirectoryPath() {
		using ns_utilities::exceptions::error_info::Description;
		namespace fs = boost::filesystem;

		std::list< std::string > testDataRoots;

		// 1. try to read the TEST_DATA_ROOT environment variable
		ns_utilities::EnvironmentVariable TEST_DATA_ROOT = getEnvironmentVariable( "TEST_DATA_ROOT" );
		if( TEST_DATA_ROOT.is_initialized() ) {
			testDataRoots.push_back( TEST_DATA_ROOT.get() );
		}

		// 2. default path to the root directory from generated header file
		const std::string defaultTestDataRoot = std::string( DEFAULT_TEST_DATA_ROOT );
		testDataRoots.push_back( defaultTestDataRoot );

		boost::optional< Path > testDataRoot = getDirectoryPath( testDataRoots );
		if( !testDataRoot ) {
			BOOST_THROW_EXCEPTION( exceptions::TestFixtureException()
				<< Description( "Invalid root directory for test data!" )
				<< Description( MAKE_STRING( "environment variable " << TEST_DATA_ROOT.getName() << ": "
						<< ( TEST_DATA_ROOT.is_initialized() ?
							TEST_DATA_ROOT.get() : "NOT SET"
						)
					))
				<< Description( MAKE_STRING( "test_utilities_config.hpp DEFAULT_TEST_DATA_ROOT: "
						<< defaultTestDataRoot
					))
			);
		}

		assert( fs::is_directory( *testDataRoot ) );
		return *testDataRoot;
	}


	/**
	 * @brief Internal helper function returning a file system path for the first existing
	 *        directory in a collection of path @c candidates.
	 *
	 * @tparam Candidates
	 *    the collection type to be used to store the candidate paths
	 *    (must be convertible to a file system path)
	 * @param[in] candidates
	 *    a collection of paths to be checked
	 * @return an optional file system path which is set to the first existing directory in
	 *         the candidate paths and is unset if none of the candidate directories exists
	 */
	template< typename Candidates >
	static boost::optional< Path > getDirectoryPath( const Candidates& candidates ) {
		namespace fs = boost::filesystem;
		boost::optional< Path > result;

		for( const auto& candidate : candidates ) {
			Path candidatePath = candidate;
			if( fs::is_directory( candidatePath ) ) {
				result = candidatePath;
				break;
			}
		}

		return result;
	}


	Path mTestDataRoot;  /**< path to the root directory storing all test data files */
	static Mutex gMutex; /**< global mutex for synchronizing access to generated data files */
};

typename TestDataFixtureBase::Mutex TestDataFixtureBase::gMutex;


/**
 * @brief Helper macro for declaring the test data directory for a test module fixture.
 *
 * This macro will generate the code to correctly define a test data @c directory for a
 * test module fixture as sub-directory of the @c fixtureBase parent module which should
 * be a base class of the module in which the declaration is used.
 *
 * @param fixtureBase
 *    the fixture base class that acts as parent module of the module to be declared
 * @param directory
 *    the name of the directory that shall be used to store test data
 */
#define TEST_MODULE_DATA_DIRECTORY( fixtureBase, directory ) \
	virtual boost::filesystem::path getTestModuleDataDirectory() const { \
		boost::filesystem::path dataDirectory = fixtureBase::getTestModuleDataDirectory(); \
		dataDirectory /= #directory; \
		return dataDirectory; \
	}


} /* end namespace ns_test_utilities */


#endif /* LIBCPP_TEST_UTILITIES_TESTDATAFIXTUREBASE_HPP_ */
