#include "V8/V8Wrapper.h"

GET_SET_DEFINITION_FLOAT(SampleStatistics, timeOfFlight);
GET_SET_DEFINITION_FLOAT(SampleStatistics, baseline);
GET_SET_DEFINITION_INT(SampleStatistics, channelNumber);
GET_SET_DEFINITION_FLOAT(SampleStatistics, minValue);
GET_SET_DEFINITION_FLOAT(SampleStatistics, timeOfMin);
GET_SET_DEFINITION_INT(SampleStatistics, indexOfMin);
GET_SET_DEFINITION_FLOAT(SampleStatistics, maxValue);
GET_SET_DEFINITION_FLOAT(SampleStatistics, timeOfMax);
GET_SET_DEFINITION_INT(SampleStatistics, indexOfMax);
GET_SET_DEFINITION_FLOAT(SampleStatistics, halfRiseValue);
GET_SET_DEFINITION_FLOAT(SampleStatistics, timeOfHalfRise);
GET_SET_DEFINITION_INT(SampleStatistics, indexOfHalfRise);
GET_SET_DEFINITION_FLOAT(SampleStatistics, timeOfCFDCrossing);
GET_SET_DEFINITION_FLOAT(SampleStatistics, deltaTprevChannelCFD);
GET_SET_DEFINITION_FLOAT(SampleStatistics, shortGateIntegral);
GET_SET_DEFINITION_FLOAT(SampleStatistics, longGateIntegral);
GET_SET_DEFINITION_FLOAT(SampleStatistics, PSD);
GET_SET_DEFINITION_FLOAT(SampleStatistics, filteredPSD);
GET_SET_DEFINITION_FLOAT(SampleStatistics, temperature);
GET_SET_DEFINITION_FLOAT(SampleStatistics, secondsFromFirstEvent);
GET_SET_DEFINITION_FLOAT(SampleStatistics, custom1);
GET_SET_DEFINITION_FLOAT(SampleStatistics, custom2);
GET_SET_DEFINITION_FLOAT(SampleStatistics, custom3);
GET_SET_DEFINITION_FLOAT(SampleStatistics, custom4);
GET_SET_DEFINITION_FLOAT(SampleStatistics, custom5);

GET_SET_DEFINITION_INT(EventStatistics, serial);
GET_SET_DEFINITION_FLOAT(EventStatistics, custom1);
GET_SET_DEFINITION_FLOAT(EventStatistics, custom2);
GET_SET_DEFINITION_FLOAT(EventStatistics, custom3);
GET_SET_DEFINITION_FLOAT(EventStatistics, custom4);
GET_SET_DEFINITION_FLOAT(EventStatistics, custom5);

void printMessage(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate=Isolate::GetCurrent();
	HandleScope handle_scope(isolate);

	 if (args.Length())
	 {
		 String::Utf8Value message( args[0]->ToString() );
         if( message.length() ) 
         {
			 qDebug()<<*message;			 
		 }
	 }
}

void setCustomParameterName(const FunctionCallbackInfo<Value>& args)
{
	Isolate* isolate=Isolate::GetCurrent();
	HandleScope handle_scope(isolate);

	 if (args.Length()>=2)
	 {
		 String::Utf8Value name( args[1]->ToString() );
		 int index=args[0]->Int32Value()-1;
		 //check if arguments are valid
         if( name.length() && index>=0 && index<5) 
         {
			 useCustomParameterName[index]=true;
			 customParameterName[index]=QString(*name);
		 }
	 }
}

 Handle<ObjectTemplate> GetSampleStatsTemplate(Isolate* isolate)
{	
	Handle<ObjectTemplate> sampleStatsTemplate = ObjectTemplate::New();
	sampleStatsTemplate->SetInternalFieldCount(1);
	SET_TEMPLATE_ACCESSOR(sampleStatsTemplate, SampleStatistics, timeOfFlight);
	SET_TEMPLATE_ACCESSOR(sampleStatsTemplate, SampleStatistics, baseline);
	SET_TEMPLATE_ACCESSOR(sampleStatsTemplate, SampleStatistics, channelNumber);
	SET_TEMPLATE_ACCESSOR(sampleStatsTemplate, SampleStatistics, minValue);
	SET_TEMPLATE_ACCESSOR(sampleStatsTemplate, SampleStatistics, timeOfMin);
	SET_TEMPLATE_ACCESSOR(sampleStatsTemplate, SampleStatistics, indexOfMin);
	SET_TEMPLATE_ACCESSOR(sampleStatsTemplate, SampleStatistics, maxValue);
	SET_TEMPLATE_ACCESSOR(sampleStatsTemplate, SampleStatistics, timeOfMax);
	SET_TEMPLATE_ACCESSOR(sampleStatsTemplate, SampleStatistics, indexOfMax);
	SET_TEMPLATE_ACCESSOR(sampleStatsTemplate, SampleStatistics, halfRiseValue);
	SET_TEMPLATE_ACCESSOR(sampleStatsTemplate, SampleStatistics, timeOfHalfRise);
	SET_TEMPLATE_ACCESSOR(sampleStatsTemplate, SampleStatistics, indexOfHalfRise);
	SET_TEMPLATE_ACCESSOR(sampleStatsTemplate, SampleStatistics, timeOfCFDCrossing);
	SET_TEMPLATE_ACCESSOR(sampleStatsTemplate, SampleStatistics, deltaTprevChannelCFD);
	SET_TEMPLATE_ACCESSOR(sampleStatsTemplate, SampleStatistics, shortGateIntegral);
	SET_TEMPLATE_ACCESSOR(sampleStatsTemplate, SampleStatistics, longGateIntegral);
	SET_TEMPLATE_ACCESSOR(sampleStatsTemplate, SampleStatistics, PSD);
	SET_TEMPLATE_ACCESSOR(sampleStatsTemplate, SampleStatistics, filteredPSD);
	SET_TEMPLATE_ACCESSOR(sampleStatsTemplate, SampleStatistics, temperature);
	SET_TEMPLATE_ACCESSOR(sampleStatsTemplate, SampleStatistics, secondsFromFirstEvent);
	SET_TEMPLATE_ACCESSOR(sampleStatsTemplate, SampleStatistics, custom1);
	SET_TEMPLATE_ACCESSOR(sampleStatsTemplate, SampleStatistics, custom2);
	SET_TEMPLATE_ACCESSOR(sampleStatsTemplate, SampleStatistics, custom3);
	SET_TEMPLATE_ACCESSOR(sampleStatsTemplate, SampleStatistics, custom4);
	SET_TEMPLATE_ACCESSOR(sampleStatsTemplate, SampleStatistics, custom5);
	return sampleStatsTemplate;
}


Handle<ObjectTemplate> GetEventStatsTemplate(Isolate* isolate)
{
	Handle<ObjectTemplate> eventStatsTemplate = ObjectTemplate::New();
	eventStatsTemplate->SetInternalFieldCount(1);
	SET_TEMPLATE_ACCESSOR(eventStatsTemplate, EventStatistics, serial);
	SET_TEMPLATE_ACCESSOR(eventStatsTemplate, EventStatistics, custom1);
	SET_TEMPLATE_ACCESSOR(eventStatsTemplate, EventStatistics, custom2);
	SET_TEMPLATE_ACCESSOR(eventStatsTemplate, EventStatistics, custom3);
	SET_TEMPLATE_ACCESSOR(eventStatsTemplate, EventStatistics, custom4);
	SET_TEMPLATE_ACCESSOR(eventStatsTemplate, EventStatistics, custom5);
	return eventStatsTemplate;
}