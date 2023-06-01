#include <algorithm>
#include <thread>
#include "./fifo_checkpoint_communicator.h"

namespace application
{
    namespace helper
    {
        const std::size_t FifoCheckpointCommunicator::cBufferSize{sizeof(uint32_t)};

        FifoCheckpointCommunicator::FifoCheckpointCommunicator(
            AsyncBsdSocketLib::Poller *poller,
            std::string fifoPath) : mPoller{poller},
                                    mClient(fifoPath),
                                    mServer(fifoPath)
        {
            bool _succeed = mServer.TrySetup();
            if (!_succeed)
            {
                throw std::runtime_error("FIFO server setup failed!");
            }

            auto _onCheckpointReceive{
                std::bind(&FifoCheckpointCommunicator::onCheckpointReceive, this)};
            _succeed = mPoller->TryAddReceiver(&mServer, _onCheckpointReceive);
            if (!_succeed)
            {
                throw std::runtime_error("Add the FIFO server to poller failed!");
            }

            _succeed = mClient.TrySetup();
            if (!_succeed)
            {
                throw std::runtime_error("FIFO client setup failed!");
            }

            auto _onCheckpointSend{
                std::bind(&FifoCheckpointCommunicator::onCheckpointSend, this)};
            _succeed = mPoller->TryAddSender(&mClient, _onCheckpointSend);
            if (!_succeed)
            {
                throw std::runtime_error("Add the FIFO client to poller failed!");
            }
        }

        void FifoCheckpointCommunicator::onCheckpointSend()
        {
            while (!mSendQueue.Empty())
            {
                std::vector<uint8_t> _payload;
                const bool cDequeued{mSendQueue.TryDequeue(_payload)};

                if (cDequeued && _payload.size() <= cBufferSize)
                {
                    auto _moveItr{
                        std::make_move_iterator(_payload.begin())};
                    std::array<uint8_t, cBufferSize> _buffer;

                    std::copy_n(
                        _moveItr,
                        _payload.size(),
                        _buffer.begin());

                    mClient.Send(_buffer);
                }

                std::this_thread::yield();
            }
        }

        void FifoCheckpointCommunicator::onCheckpointReceive()
        {
            std::array<uint8_t, cBufferSize> _buffer;
            const bool cSuccessful{mServer.Receive(_buffer) > 0};

            if (cSuccessful && Callback)
            {
                const std::vector<uint8_t> cPayload(
                    _buffer.cbegin(), _buffer.cend());

                std::size_t _offset = 0;
                const uint32_t cCheckpoint{
                    ara::com::helper::ExtractInteger(cPayload, _offset)};

                Callback(cCheckpoint);
            }
        }

        bool FifoCheckpointCommunicator::TrySend(uint32_t checkpoint)
        {
            std::vector<uint8_t> _payload;
            ara::com::helper::Inject(_payload, checkpoint);
            const bool cResult{mSendQueue.TryEnqueue(std::move(_payload))};

            return cResult;
        }

        FifoCheckpointCommunicator::~FifoCheckpointCommunicator()
        {
            mPoller->TryRemoveSender(&mClient);
            mPoller->TryRemoveReceiver(&mServer);
        }
    }
}