#pragma once
#include <include/v8.h>
#include <include/libplatform/libplatform.h>
#include "AcquisitionDefinitions.h"

using namespace v8;


class ArrayBufferAllocator : public v8::ArrayBuffer::Allocator {
 public:
  virtual void* Allocate(size_t length) {
    void* data = AllocateUninitialized(length);
    return data == NULL ? data : memset(data, 0, length);
  }
  virtual void* AllocateUninitialized(size_t length) { return malloc(length); }
  virtual void Free(void* data, size_t) { free(data); }
};
#define GET_SET_DECLARATION(className, varName) \
	void get_##className##varName (Local<String>property,const PropertyCallbackInfo<Value> &info); \
	void set_##className##varName (Local<String>property,Local<Value> value, const PropertyCallbackInfo<void> &info); 

#define GET_SET_DEFINITION_FLOAT(className, varName) \
	void get_##className##varName (Local<String>property,const PropertyCallbackInfo<Value> &info) \
	 {	\
		Local<Object> self = info.Holder(); \
		Local<External> wrap = Local<External>::Cast(self->GetInternalField(0)); \
		void* ptr = wrap->Value(); \
		double value = static_cast<className*>(ptr)->varName; \
		info.GetReturnValue().Set(value); \
	  } \
	  void set_##className##varName (Local<String>property,Local<Value> value, const PropertyCallbackInfo<void> &info) \
	  {\
		Local<Object> self = info.Holder(); \
		Local<External> wrap = Local<External>::Cast(self->GetInternalField(0)); \
		void* ptr = wrap->Value(); \
		static_cast<className*>(ptr)->varName = (float) value->NumberValue(); \
	}

#define GET_SET_DEFINITION_INT(className, varName) \
	void get_##className##varName (Local<String>property,const PropertyCallbackInfo<Value> &info) \
	 {	\
		Local<Object> self = info.Holder(); \
		Local<External> wrap = Local<External>::Cast(self->GetInternalField(0)); \
		void* ptr = wrap->Value(); \
		double value = static_cast<className*>(ptr)->varName; \
		info.GetReturnValue().Set(value); \
	  } \
	  void set_##className##varName (Local<String>property,Local<Value> value, const PropertyCallbackInfo<void> &info) \
	  {\
		Local<Object> self = info.Holder(); \
		Local<External> wrap = Local<External>::Cast(self->GetInternalField(0)); \
		void* ptr = wrap->Value(); \
		static_cast<className*>(ptr)->varName = (float) value->Int32Value(); \
	}

#define SET_TEMPLATE_ACCESSOR(templateVar, className, varName) \
	(templateVar)->SetAccessor(String::NewFromUtf8(isolate, #varName), get_##className##varName, set_##className##varName);


GET_SET_DECLARATION(SampleStatistics, timeOfFlight);
GET_SET_DECLARATION(SampleStatistics, baseline);
GET_SET_DECLARATION(SampleStatistics, channelNumber);
GET_SET_DECLARATION(SampleStatistics, minValue);
GET_SET_DECLARATION(SampleStatistics, timeOfMin);
GET_SET_DECLARATION(SampleStatistics, indexOfMin);
GET_SET_DECLARATION(SampleStatistics, maxValue);
GET_SET_DECLARATION(SampleStatistics, timeOfMax);
GET_SET_DECLARATION(SampleStatistics, indexOfMax);
GET_SET_DECLARATION(SampleStatistics, halfRiseValue);
GET_SET_DECLARATION(SampleStatistics, timeOfHalfRise);
GET_SET_DECLARATION(SampleStatistics, indexOfHalfRise);
GET_SET_DECLARATION(SampleStatistics, timeOfCFDCrossing);
GET_SET_DECLARATION(SampleStatistics, deltaTprevChannelCFD);
GET_SET_DECLARATION(SampleStatistics, shortGateIntegral);
GET_SET_DECLARATION(SampleStatistics, longGateIntegral);
GET_SET_DECLARATION(SampleStatistics, PSD);
GET_SET_DECLARATION(SampleStatistics, filteredPSD);
GET_SET_DECLARATION(SampleStatistics, temperature);
GET_SET_DECLARATION(SampleStatistics, secondsFromFirstEvent);
GET_SET_DECLARATION(SampleStatistics, custom1);
GET_SET_DECLARATION(SampleStatistics, custom2);
GET_SET_DECLARATION(SampleStatistics, custom3);
GET_SET_DECLARATION(SampleStatistics, custom4);
GET_SET_DECLARATION(SampleStatistics, custom5);

GET_SET_DECLARATION(EventStatistics, serial);
GET_SET_DECLARATION(EventStatistics, custom1);
GET_SET_DECLARATION(EventStatistics, custom2);
GET_SET_DECLARATION(EventStatistics, custom3);
GET_SET_DECLARATION(EventStatistics, custom4);
GET_SET_DECLARATION(EventStatistics, custom5);

void printMessage(const FunctionCallbackInfo<Value>& args);
void setCustomParameterName(const FunctionCallbackInfo<Value>& args);

Handle<ObjectTemplate> GetSampleStatsTemplate(Isolate* isolate);
Handle<ObjectTemplate> GetEventStatsTemplate(Isolate* isolate);

void checkV8Exceptions(v8::TryCatch &try_catch, QString codeblockName = "General", bool showErrorBox = true);
int getCompileErrorLine(v8::TryCatch &try_catch, QString& message);
