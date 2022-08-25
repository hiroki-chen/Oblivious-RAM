// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: messages.proto

#include "messages.pb.h"
#include "messages.grpc.pb.h"

#include <functional>
#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/channel_interface.h>
#include <grpcpp/impl/codegen/client_unary_call.h>
#include <grpcpp/impl/codegen/client_callback.h>
#include <grpcpp/impl/codegen/message_allocator.h>
#include <grpcpp/impl/codegen/method_handler.h>
#include <grpcpp/impl/codegen/rpc_service_method.h>
#include <grpcpp/impl/codegen/server_callback.h>
#include <grpcpp/impl/codegen/server_callback_handlers.h>
#include <grpcpp/impl/codegen/server_context.h>
#include <grpcpp/impl/codegen/service_type.h>
#include <grpcpp/impl/codegen/sync_stream.h>
namespace oram_impl {

static const char* oram_server_method_names[] = {
  "/oram_impl.oram_server/InitTreeOram",
  "/oram_impl.oram_server/InitFlatOram",
  "/oram_impl.oram_server/PrintOramTree",
  "/oram_impl.oram_server/ReadPath",
  "/oram_impl.oram_server/WritePath",
  "/oram_impl.oram_server/ReadFlatMemory",
  "/oram_impl.oram_server/WriteFlatMemory",
  "/oram_impl.oram_server/CloseConnection",
  "/oram_impl.oram_server/KeyExchange",
  "/oram_impl.oram_server/SendHello",
  "/oram_impl.oram_server/ReportServerInformation",
  "/oram_impl.oram_server/ResetServer",
};

std::unique_ptr< oram_server::Stub> oram_server::NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options) {
  (void)options;
  std::unique_ptr< oram_server::Stub> stub(new oram_server::Stub(channel, options));
  return stub;
}

oram_server::Stub::Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options)
  : channel_(channel), rpcmethod_InitTreeOram_(oram_server_method_names[0], options.suffix_for_stats(),::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_InitFlatOram_(oram_server_method_names[1], options.suffix_for_stats(),::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_PrintOramTree_(oram_server_method_names[2], options.suffix_for_stats(),::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_ReadPath_(oram_server_method_names[3], options.suffix_for_stats(),::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_WritePath_(oram_server_method_names[4], options.suffix_for_stats(),::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_ReadFlatMemory_(oram_server_method_names[5], options.suffix_for_stats(),::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_WriteFlatMemory_(oram_server_method_names[6], options.suffix_for_stats(),::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_CloseConnection_(oram_server_method_names[7], options.suffix_for_stats(),::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_KeyExchange_(oram_server_method_names[8], options.suffix_for_stats(),::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_SendHello_(oram_server_method_names[9], options.suffix_for_stats(),::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_ReportServerInformation_(oram_server_method_names[10], options.suffix_for_stats(),::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_ResetServer_(oram_server_method_names[11], options.suffix_for_stats(),::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  {}

::grpc::Status oram_server::Stub::InitTreeOram(::grpc::ClientContext* context, const ::oram_impl::InitTreeOramRequest& request, ::google::protobuf::Empty* response) {
  return ::grpc::internal::BlockingUnaryCall< ::oram_impl::InitTreeOramRequest, ::google::protobuf::Empty, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), rpcmethod_InitTreeOram_, context, request, response);
}

void oram_server::Stub::async::InitTreeOram(::grpc::ClientContext* context, const ::oram_impl::InitTreeOramRequest* request, ::google::protobuf::Empty* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall< ::oram_impl::InitTreeOramRequest, ::google::protobuf::Empty, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_InitTreeOram_, context, request, response, std::move(f));
}

void oram_server::Stub::async::InitTreeOram(::grpc::ClientContext* context, const ::oram_impl::InitTreeOramRequest* request, ::google::protobuf::Empty* response, ::grpc::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create< ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_InitTreeOram_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::google::protobuf::Empty>* oram_server::Stub::PrepareAsyncInitTreeOramRaw(::grpc::ClientContext* context, const ::oram_impl::InitTreeOramRequest& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderHelper::Create< ::google::protobuf::Empty, ::oram_impl::InitTreeOramRequest, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), cq, rpcmethod_InitTreeOram_, context, request);
}

::grpc::ClientAsyncResponseReader< ::google::protobuf::Empty>* oram_server::Stub::AsyncInitTreeOramRaw(::grpc::ClientContext* context, const ::oram_impl::InitTreeOramRequest& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncInitTreeOramRaw(context, request, cq);
  result->StartCall();
  return result;
}

::grpc::Status oram_server::Stub::InitFlatOram(::grpc::ClientContext* context, const ::oram_impl::InitFlatOramRequest& request, ::google::protobuf::Empty* response) {
  return ::grpc::internal::BlockingUnaryCall< ::oram_impl::InitFlatOramRequest, ::google::protobuf::Empty, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), rpcmethod_InitFlatOram_, context, request, response);
}

void oram_server::Stub::async::InitFlatOram(::grpc::ClientContext* context, const ::oram_impl::InitFlatOramRequest* request, ::google::protobuf::Empty* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall< ::oram_impl::InitFlatOramRequest, ::google::protobuf::Empty, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_InitFlatOram_, context, request, response, std::move(f));
}

void oram_server::Stub::async::InitFlatOram(::grpc::ClientContext* context, const ::oram_impl::InitFlatOramRequest* request, ::google::protobuf::Empty* response, ::grpc::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create< ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_InitFlatOram_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::google::protobuf::Empty>* oram_server::Stub::PrepareAsyncInitFlatOramRaw(::grpc::ClientContext* context, const ::oram_impl::InitFlatOramRequest& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderHelper::Create< ::google::protobuf::Empty, ::oram_impl::InitFlatOramRequest, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), cq, rpcmethod_InitFlatOram_, context, request);
}

::grpc::ClientAsyncResponseReader< ::google::protobuf::Empty>* oram_server::Stub::AsyncInitFlatOramRaw(::grpc::ClientContext* context, const ::oram_impl::InitFlatOramRequest& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncInitFlatOramRaw(context, request, cq);
  result->StartCall();
  return result;
}

::grpc::Status oram_server::Stub::PrintOramTree(::grpc::ClientContext* context, const ::oram_impl::PrintOramTreeRequest& request, ::google::protobuf::Empty* response) {
  return ::grpc::internal::BlockingUnaryCall< ::oram_impl::PrintOramTreeRequest, ::google::protobuf::Empty, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), rpcmethod_PrintOramTree_, context, request, response);
}

void oram_server::Stub::async::PrintOramTree(::grpc::ClientContext* context, const ::oram_impl::PrintOramTreeRequest* request, ::google::protobuf::Empty* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall< ::oram_impl::PrintOramTreeRequest, ::google::protobuf::Empty, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_PrintOramTree_, context, request, response, std::move(f));
}

void oram_server::Stub::async::PrintOramTree(::grpc::ClientContext* context, const ::oram_impl::PrintOramTreeRequest* request, ::google::protobuf::Empty* response, ::grpc::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create< ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_PrintOramTree_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::google::protobuf::Empty>* oram_server::Stub::PrepareAsyncPrintOramTreeRaw(::grpc::ClientContext* context, const ::oram_impl::PrintOramTreeRequest& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderHelper::Create< ::google::protobuf::Empty, ::oram_impl::PrintOramTreeRequest, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), cq, rpcmethod_PrintOramTree_, context, request);
}

::grpc::ClientAsyncResponseReader< ::google::protobuf::Empty>* oram_server::Stub::AsyncPrintOramTreeRaw(::grpc::ClientContext* context, const ::oram_impl::PrintOramTreeRequest& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncPrintOramTreeRaw(context, request, cq);
  result->StartCall();
  return result;
}

::grpc::Status oram_server::Stub::ReadPath(::grpc::ClientContext* context, const ::oram_impl::ReadPathRequest& request, ::oram_impl::ReadPathResponse* response) {
  return ::grpc::internal::BlockingUnaryCall< ::oram_impl::ReadPathRequest, ::oram_impl::ReadPathResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), rpcmethod_ReadPath_, context, request, response);
}

void oram_server::Stub::async::ReadPath(::grpc::ClientContext* context, const ::oram_impl::ReadPathRequest* request, ::oram_impl::ReadPathResponse* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall< ::oram_impl::ReadPathRequest, ::oram_impl::ReadPathResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_ReadPath_, context, request, response, std::move(f));
}

void oram_server::Stub::async::ReadPath(::grpc::ClientContext* context, const ::oram_impl::ReadPathRequest* request, ::oram_impl::ReadPathResponse* response, ::grpc::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create< ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_ReadPath_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::oram_impl::ReadPathResponse>* oram_server::Stub::PrepareAsyncReadPathRaw(::grpc::ClientContext* context, const ::oram_impl::ReadPathRequest& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderHelper::Create< ::oram_impl::ReadPathResponse, ::oram_impl::ReadPathRequest, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), cq, rpcmethod_ReadPath_, context, request);
}

::grpc::ClientAsyncResponseReader< ::oram_impl::ReadPathResponse>* oram_server::Stub::AsyncReadPathRaw(::grpc::ClientContext* context, const ::oram_impl::ReadPathRequest& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncReadPathRaw(context, request, cq);
  result->StartCall();
  return result;
}

::grpc::Status oram_server::Stub::WritePath(::grpc::ClientContext* context, const ::oram_impl::WritePathRequest& request, ::oram_impl::WritePathResponse* response) {
  return ::grpc::internal::BlockingUnaryCall< ::oram_impl::WritePathRequest, ::oram_impl::WritePathResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), rpcmethod_WritePath_, context, request, response);
}

void oram_server::Stub::async::WritePath(::grpc::ClientContext* context, const ::oram_impl::WritePathRequest* request, ::oram_impl::WritePathResponse* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall< ::oram_impl::WritePathRequest, ::oram_impl::WritePathResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_WritePath_, context, request, response, std::move(f));
}

void oram_server::Stub::async::WritePath(::grpc::ClientContext* context, const ::oram_impl::WritePathRequest* request, ::oram_impl::WritePathResponse* response, ::grpc::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create< ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_WritePath_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::oram_impl::WritePathResponse>* oram_server::Stub::PrepareAsyncWritePathRaw(::grpc::ClientContext* context, const ::oram_impl::WritePathRequest& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderHelper::Create< ::oram_impl::WritePathResponse, ::oram_impl::WritePathRequest, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), cq, rpcmethod_WritePath_, context, request);
}

::grpc::ClientAsyncResponseReader< ::oram_impl::WritePathResponse>* oram_server::Stub::AsyncWritePathRaw(::grpc::ClientContext* context, const ::oram_impl::WritePathRequest& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncWritePathRaw(context, request, cq);
  result->StartCall();
  return result;
}

::grpc::Status oram_server::Stub::ReadFlatMemory(::grpc::ClientContext* context, const ::oram_impl::ReadFlatRequest& request, ::oram_impl::FlatVectorMessage* response) {
  return ::grpc::internal::BlockingUnaryCall< ::oram_impl::ReadFlatRequest, ::oram_impl::FlatVectorMessage, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), rpcmethod_ReadFlatMemory_, context, request, response);
}

void oram_server::Stub::async::ReadFlatMemory(::grpc::ClientContext* context, const ::oram_impl::ReadFlatRequest* request, ::oram_impl::FlatVectorMessage* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall< ::oram_impl::ReadFlatRequest, ::oram_impl::FlatVectorMessage, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_ReadFlatMemory_, context, request, response, std::move(f));
}

void oram_server::Stub::async::ReadFlatMemory(::grpc::ClientContext* context, const ::oram_impl::ReadFlatRequest* request, ::oram_impl::FlatVectorMessage* response, ::grpc::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create< ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_ReadFlatMemory_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::oram_impl::FlatVectorMessage>* oram_server::Stub::PrepareAsyncReadFlatMemoryRaw(::grpc::ClientContext* context, const ::oram_impl::ReadFlatRequest& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderHelper::Create< ::oram_impl::FlatVectorMessage, ::oram_impl::ReadFlatRequest, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), cq, rpcmethod_ReadFlatMemory_, context, request);
}

::grpc::ClientAsyncResponseReader< ::oram_impl::FlatVectorMessage>* oram_server::Stub::AsyncReadFlatMemoryRaw(::grpc::ClientContext* context, const ::oram_impl::ReadFlatRequest& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncReadFlatMemoryRaw(context, request, cq);
  result->StartCall();
  return result;
}

::grpc::Status oram_server::Stub::WriteFlatMemory(::grpc::ClientContext* context, const ::oram_impl::FlatVectorMessage& request, ::google::protobuf::Empty* response) {
  return ::grpc::internal::BlockingUnaryCall< ::oram_impl::FlatVectorMessage, ::google::protobuf::Empty, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), rpcmethod_WriteFlatMemory_, context, request, response);
}

void oram_server::Stub::async::WriteFlatMemory(::grpc::ClientContext* context, const ::oram_impl::FlatVectorMessage* request, ::google::protobuf::Empty* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall< ::oram_impl::FlatVectorMessage, ::google::protobuf::Empty, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_WriteFlatMemory_, context, request, response, std::move(f));
}

void oram_server::Stub::async::WriteFlatMemory(::grpc::ClientContext* context, const ::oram_impl::FlatVectorMessage* request, ::google::protobuf::Empty* response, ::grpc::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create< ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_WriteFlatMemory_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::google::protobuf::Empty>* oram_server::Stub::PrepareAsyncWriteFlatMemoryRaw(::grpc::ClientContext* context, const ::oram_impl::FlatVectorMessage& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderHelper::Create< ::google::protobuf::Empty, ::oram_impl::FlatVectorMessage, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), cq, rpcmethod_WriteFlatMemory_, context, request);
}

::grpc::ClientAsyncResponseReader< ::google::protobuf::Empty>* oram_server::Stub::AsyncWriteFlatMemoryRaw(::grpc::ClientContext* context, const ::oram_impl::FlatVectorMessage& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncWriteFlatMemoryRaw(context, request, cq);
  result->StartCall();
  return result;
}

::grpc::Status oram_server::Stub::CloseConnection(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::google::protobuf::Empty* response) {
  return ::grpc::internal::BlockingUnaryCall< ::google::protobuf::Empty, ::google::protobuf::Empty, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), rpcmethod_CloseConnection_, context, request, response);
}

void oram_server::Stub::async::CloseConnection(::grpc::ClientContext* context, const ::google::protobuf::Empty* request, ::google::protobuf::Empty* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall< ::google::protobuf::Empty, ::google::protobuf::Empty, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_CloseConnection_, context, request, response, std::move(f));
}

void oram_server::Stub::async::CloseConnection(::grpc::ClientContext* context, const ::google::protobuf::Empty* request, ::google::protobuf::Empty* response, ::grpc::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create< ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_CloseConnection_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::google::protobuf::Empty>* oram_server::Stub::PrepareAsyncCloseConnectionRaw(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderHelper::Create< ::google::protobuf::Empty, ::google::protobuf::Empty, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), cq, rpcmethod_CloseConnection_, context, request);
}

::grpc::ClientAsyncResponseReader< ::google::protobuf::Empty>* oram_server::Stub::AsyncCloseConnectionRaw(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncCloseConnectionRaw(context, request, cq);
  result->StartCall();
  return result;
}

::grpc::Status oram_server::Stub::KeyExchange(::grpc::ClientContext* context, const ::oram_impl::KeyExchangeRequest& request, ::oram_impl::KeyExchangeResponse* response) {
  return ::grpc::internal::BlockingUnaryCall< ::oram_impl::KeyExchangeRequest, ::oram_impl::KeyExchangeResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), rpcmethod_KeyExchange_, context, request, response);
}

void oram_server::Stub::async::KeyExchange(::grpc::ClientContext* context, const ::oram_impl::KeyExchangeRequest* request, ::oram_impl::KeyExchangeResponse* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall< ::oram_impl::KeyExchangeRequest, ::oram_impl::KeyExchangeResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_KeyExchange_, context, request, response, std::move(f));
}

void oram_server::Stub::async::KeyExchange(::grpc::ClientContext* context, const ::oram_impl::KeyExchangeRequest* request, ::oram_impl::KeyExchangeResponse* response, ::grpc::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create< ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_KeyExchange_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::oram_impl::KeyExchangeResponse>* oram_server::Stub::PrepareAsyncKeyExchangeRaw(::grpc::ClientContext* context, const ::oram_impl::KeyExchangeRequest& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderHelper::Create< ::oram_impl::KeyExchangeResponse, ::oram_impl::KeyExchangeRequest, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), cq, rpcmethod_KeyExchange_, context, request);
}

::grpc::ClientAsyncResponseReader< ::oram_impl::KeyExchangeResponse>* oram_server::Stub::AsyncKeyExchangeRaw(::grpc::ClientContext* context, const ::oram_impl::KeyExchangeRequest& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncKeyExchangeRaw(context, request, cq);
  result->StartCall();
  return result;
}

::grpc::Status oram_server::Stub::SendHello(::grpc::ClientContext* context, const ::oram_impl::HelloMessage& request, ::google::protobuf::Empty* response) {
  return ::grpc::internal::BlockingUnaryCall< ::oram_impl::HelloMessage, ::google::protobuf::Empty, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), rpcmethod_SendHello_, context, request, response);
}

void oram_server::Stub::async::SendHello(::grpc::ClientContext* context, const ::oram_impl::HelloMessage* request, ::google::protobuf::Empty* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall< ::oram_impl::HelloMessage, ::google::protobuf::Empty, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_SendHello_, context, request, response, std::move(f));
}

void oram_server::Stub::async::SendHello(::grpc::ClientContext* context, const ::oram_impl::HelloMessage* request, ::google::protobuf::Empty* response, ::grpc::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create< ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_SendHello_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::google::protobuf::Empty>* oram_server::Stub::PrepareAsyncSendHelloRaw(::grpc::ClientContext* context, const ::oram_impl::HelloMessage& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderHelper::Create< ::google::protobuf::Empty, ::oram_impl::HelloMessage, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), cq, rpcmethod_SendHello_, context, request);
}

::grpc::ClientAsyncResponseReader< ::google::protobuf::Empty>* oram_server::Stub::AsyncSendHelloRaw(::grpc::ClientContext* context, const ::oram_impl::HelloMessage& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncSendHelloRaw(context, request, cq);
  result->StartCall();
  return result;
}

::grpc::Status oram_server::Stub::ReportServerInformation(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::google::protobuf::Empty* response) {
  return ::grpc::internal::BlockingUnaryCall< ::google::protobuf::Empty, ::google::protobuf::Empty, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), rpcmethod_ReportServerInformation_, context, request, response);
}

void oram_server::Stub::async::ReportServerInformation(::grpc::ClientContext* context, const ::google::protobuf::Empty* request, ::google::protobuf::Empty* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall< ::google::protobuf::Empty, ::google::protobuf::Empty, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_ReportServerInformation_, context, request, response, std::move(f));
}

void oram_server::Stub::async::ReportServerInformation(::grpc::ClientContext* context, const ::google::protobuf::Empty* request, ::google::protobuf::Empty* response, ::grpc::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create< ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_ReportServerInformation_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::google::protobuf::Empty>* oram_server::Stub::PrepareAsyncReportServerInformationRaw(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderHelper::Create< ::google::protobuf::Empty, ::google::protobuf::Empty, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), cq, rpcmethod_ReportServerInformation_, context, request);
}

::grpc::ClientAsyncResponseReader< ::google::protobuf::Empty>* oram_server::Stub::AsyncReportServerInformationRaw(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncReportServerInformationRaw(context, request, cq);
  result->StartCall();
  return result;
}

::grpc::Status oram_server::Stub::ResetServer(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::google::protobuf::Empty* response) {
  return ::grpc::internal::BlockingUnaryCall< ::google::protobuf::Empty, ::google::protobuf::Empty, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), rpcmethod_ResetServer_, context, request, response);
}

void oram_server::Stub::async::ResetServer(::grpc::ClientContext* context, const ::google::protobuf::Empty* request, ::google::protobuf::Empty* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall< ::google::protobuf::Empty, ::google::protobuf::Empty, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_ResetServer_, context, request, response, std::move(f));
}

void oram_server::Stub::async::ResetServer(::grpc::ClientContext* context, const ::google::protobuf::Empty* request, ::google::protobuf::Empty* response, ::grpc::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create< ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_ResetServer_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::google::protobuf::Empty>* oram_server::Stub::PrepareAsyncResetServerRaw(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderHelper::Create< ::google::protobuf::Empty, ::google::protobuf::Empty, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), cq, rpcmethod_ResetServer_, context, request);
}

::grpc::ClientAsyncResponseReader< ::google::protobuf::Empty>* oram_server::Stub::AsyncResetServerRaw(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncResetServerRaw(context, request, cq);
  result->StartCall();
  return result;
}

oram_server::Service::Service() {
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      oram_server_method_names[0],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< oram_server::Service, ::oram_impl::InitTreeOramRequest, ::google::protobuf::Empty, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(
          [](oram_server::Service* service,
             ::grpc::ServerContext* ctx,
             const ::oram_impl::InitTreeOramRequest* req,
             ::google::protobuf::Empty* resp) {
               return service->InitTreeOram(ctx, req, resp);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      oram_server_method_names[1],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< oram_server::Service, ::oram_impl::InitFlatOramRequest, ::google::protobuf::Empty, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(
          [](oram_server::Service* service,
             ::grpc::ServerContext* ctx,
             const ::oram_impl::InitFlatOramRequest* req,
             ::google::protobuf::Empty* resp) {
               return service->InitFlatOram(ctx, req, resp);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      oram_server_method_names[2],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< oram_server::Service, ::oram_impl::PrintOramTreeRequest, ::google::protobuf::Empty, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(
          [](oram_server::Service* service,
             ::grpc::ServerContext* ctx,
             const ::oram_impl::PrintOramTreeRequest* req,
             ::google::protobuf::Empty* resp) {
               return service->PrintOramTree(ctx, req, resp);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      oram_server_method_names[3],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< oram_server::Service, ::oram_impl::ReadPathRequest, ::oram_impl::ReadPathResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(
          [](oram_server::Service* service,
             ::grpc::ServerContext* ctx,
             const ::oram_impl::ReadPathRequest* req,
             ::oram_impl::ReadPathResponse* resp) {
               return service->ReadPath(ctx, req, resp);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      oram_server_method_names[4],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< oram_server::Service, ::oram_impl::WritePathRequest, ::oram_impl::WritePathResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(
          [](oram_server::Service* service,
             ::grpc::ServerContext* ctx,
             const ::oram_impl::WritePathRequest* req,
             ::oram_impl::WritePathResponse* resp) {
               return service->WritePath(ctx, req, resp);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      oram_server_method_names[5],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< oram_server::Service, ::oram_impl::ReadFlatRequest, ::oram_impl::FlatVectorMessage, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(
          [](oram_server::Service* service,
             ::grpc::ServerContext* ctx,
             const ::oram_impl::ReadFlatRequest* req,
             ::oram_impl::FlatVectorMessage* resp) {
               return service->ReadFlatMemory(ctx, req, resp);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      oram_server_method_names[6],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< oram_server::Service, ::oram_impl::FlatVectorMessage, ::google::protobuf::Empty, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(
          [](oram_server::Service* service,
             ::grpc::ServerContext* ctx,
             const ::oram_impl::FlatVectorMessage* req,
             ::google::protobuf::Empty* resp) {
               return service->WriteFlatMemory(ctx, req, resp);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      oram_server_method_names[7],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< oram_server::Service, ::google::protobuf::Empty, ::google::protobuf::Empty, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(
          [](oram_server::Service* service,
             ::grpc::ServerContext* ctx,
             const ::google::protobuf::Empty* req,
             ::google::protobuf::Empty* resp) {
               return service->CloseConnection(ctx, req, resp);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      oram_server_method_names[8],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< oram_server::Service, ::oram_impl::KeyExchangeRequest, ::oram_impl::KeyExchangeResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(
          [](oram_server::Service* service,
             ::grpc::ServerContext* ctx,
             const ::oram_impl::KeyExchangeRequest* req,
             ::oram_impl::KeyExchangeResponse* resp) {
               return service->KeyExchange(ctx, req, resp);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      oram_server_method_names[9],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< oram_server::Service, ::oram_impl::HelloMessage, ::google::protobuf::Empty, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(
          [](oram_server::Service* service,
             ::grpc::ServerContext* ctx,
             const ::oram_impl::HelloMessage* req,
             ::google::protobuf::Empty* resp) {
               return service->SendHello(ctx, req, resp);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      oram_server_method_names[10],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< oram_server::Service, ::google::protobuf::Empty, ::google::protobuf::Empty, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(
          [](oram_server::Service* service,
             ::grpc::ServerContext* ctx,
             const ::google::protobuf::Empty* req,
             ::google::protobuf::Empty* resp) {
               return service->ReportServerInformation(ctx, req, resp);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      oram_server_method_names[11],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< oram_server::Service, ::google::protobuf::Empty, ::google::protobuf::Empty, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(
          [](oram_server::Service* service,
             ::grpc::ServerContext* ctx,
             const ::google::protobuf::Empty* req,
             ::google::protobuf::Empty* resp) {
               return service->ResetServer(ctx, req, resp);
             }, this)));
}

oram_server::Service::~Service() {
}

::grpc::Status oram_server::Service::InitTreeOram(::grpc::ServerContext* context, const ::oram_impl::InitTreeOramRequest* request, ::google::protobuf::Empty* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status oram_server::Service::InitFlatOram(::grpc::ServerContext* context, const ::oram_impl::InitFlatOramRequest* request, ::google::protobuf::Empty* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status oram_server::Service::PrintOramTree(::grpc::ServerContext* context, const ::oram_impl::PrintOramTreeRequest* request, ::google::protobuf::Empty* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status oram_server::Service::ReadPath(::grpc::ServerContext* context, const ::oram_impl::ReadPathRequest* request, ::oram_impl::ReadPathResponse* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status oram_server::Service::WritePath(::grpc::ServerContext* context, const ::oram_impl::WritePathRequest* request, ::oram_impl::WritePathResponse* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status oram_server::Service::ReadFlatMemory(::grpc::ServerContext* context, const ::oram_impl::ReadFlatRequest* request, ::oram_impl::FlatVectorMessage* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status oram_server::Service::WriteFlatMemory(::grpc::ServerContext* context, const ::oram_impl::FlatVectorMessage* request, ::google::protobuf::Empty* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status oram_server::Service::CloseConnection(::grpc::ServerContext* context, const ::google::protobuf::Empty* request, ::google::protobuf::Empty* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status oram_server::Service::KeyExchange(::grpc::ServerContext* context, const ::oram_impl::KeyExchangeRequest* request, ::oram_impl::KeyExchangeResponse* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status oram_server::Service::SendHello(::grpc::ServerContext* context, const ::oram_impl::HelloMessage* request, ::google::protobuf::Empty* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status oram_server::Service::ReportServerInformation(::grpc::ServerContext* context, const ::google::protobuf::Empty* request, ::google::protobuf::Empty* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status oram_server::Service::ResetServer(::grpc::ServerContext* context, const ::google::protobuf::Empty* request, ::google::protobuf::Empty* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}


}  // namespace oram_impl

