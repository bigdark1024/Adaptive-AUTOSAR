#ifndef MODELLED_PROCESS_H
#define MODELLED_PROCESS_H

#include <atomic>
#include <future>
#include <map>
#include <string>

namespace ara
{
    namespace exec
    {
        namespace helper
        {
            /// @brief A class that models an instance of an Adaptive (Platform) Application executable
            class ModelledProcess
            {
            private:
                std::atomic_bool mCancellationToken;
                std::future<int> mExitCode;

            protected:
                ModelledProcess() noexcept;

                /// @brief Main running block of the process
                /// @param arguments Initialization arguments keys and their corresponding values
                /// @return Exit code
                /// @note Exit code zero means the graceful shutdown of the process.
                virtual int Main(
                    const std::atomic_bool *cancellationToken,
                    const std::map<std::string, std::string> &arguments) = 0;

            public:
                /// @brief Initialize the process model to run the main block
                /// @param arguments Initialization arguments keys and their corresponding values
                /// @see Main(const std::map<std::string, std::string> &)
                void Initialize(const std::map<std::string, std::string> &arguments);

                /// @brief Terminate the process model
                /// @return Returned exit code from the main running block
                /// @note The caller will blocked until the termination be finished.
                int Terminate();

                virtual ~ModelledProcess();
            };
        }
    }
}

#endif