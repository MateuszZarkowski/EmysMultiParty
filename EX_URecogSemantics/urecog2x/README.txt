*******************************************
*
*   	URecog
*   	Simple module to SR based on Microsoft Speech SDK
*   	author: 
*				Jan Kedzierski, 
*				Mateusz Zarkowski
*				@Wroclaw University of Technology
*   	  date: 17.06.2014
*	   ver: 2.76
*
*******************************************
*/

Description:
//////////////////////////////////////////
URBI module for speech recognition based on Microsoft Speech SDK.
The Microsoft Speech SDK consists of a Software Development Kit, 
a Runtime, and Runtime Languages (language packs that enable speech 
recognition or text-to-speech for a specific language). 

Grammars are at the core of speech recognition and are perhaps the most 
important component under control of the speech application developer that 
affects the accuracy of speech recognition. Grammars work in conjunction 
with the speech recognition engine and its lexicons and speech models to 
define the factors that affect speech recognition performance.

The Microsoft Speech Platform SDK provides programmatic processes for 
authoring speech recognition grammars and also offers support for XML-format 
grammars authored in compliance with industry standards.
 
 
More about MSP:
http://msdn.microsoft.com/en-us/library/office/hh361572(v=office.14).aspx
MSP SDK
http://www.microsoft.com/en-us/download/details.aspx?id=27226
MSP Runtime
http://www.microsoft.com/en-us/download/details.aspx?id=27225
MSP Languages
http://www.microsoft.com/en-us/download/details.aspx?id=27224
  
More about Microsoft Speech Platform Grammars:
http://msdn.microsoft.com/en-us/library/hh378481(v=office.14).aspx

//////////////////////////////////////////
Software requirements:
//////////////////////////////////////////
	for running module:
	- Microsoft Speech Platform Runtime,
	for compilation it is needed additional libraries:
	- Microsoft Speech Platform SDK (tested with 11.0).
	
//////////////////////////////////////////
Module functions:
//////////////////////////////////////////
URecog.new("engine", recognizerNo, inputNo); - initialize SR engine,
	engine - you can choose "InProc" In-process or "InShared" shared engine,
	recognizerNo - choose input number (mic, Kinect,...),
	inputNo - choose recognizer (language),
URecog.AvailableRecognizers; - get all available recognizers,
URecog.AvailableInputs; - get all available inputs,
URecog.Poll(bool); - poll speech recognition stream, set true to wait for result or false to check events only,
URecog.AddPhrase(rule, word); - add dynamicaly sentance to recognition,
URecog.ResetGrammar(); - clear all grammar rules
URrgog.ClearResult(); - clear recognition results
URecog.SetDictationState(SPRULESTATE); - sets the dictation topic state. 
	Flag of type SPRULESTATE indicating the new state of dictation. 
	0 - SPRS_INACTIVE
	1 - SPRS_ACTIVE
	3 - SPRS_ACTIVE_WITH_AUTO_PAUSE
	4 - SPRS_ACTIVE_USER_DELIMITED
URecog.pause; - pause recognition engine, true - paused, false - resume,
URecog.result - recognition result,
URecog.resultTag - recognition result tag,
URecog.semantics - dictionary with recognized semantic tags and their values
URecog.confidence - recognition result confidence value,
URecog.confidenceTreshold - recognition confidence treshold value,
URecog.isListening - recognition engine status flag.


//////////////////////////////////////////
Urbiscript example 1:

loadModule("URecog");
var recog=URecog.new("InProc",0,0);

recog.AddPhrase("","my name is John");
recog.AddPhrase("","please start game");
recog.AddPhrase("","yes");
recog.AddPhrase("","no");
recog.AddPhrase("","stop game");
recog.AddPhrase("","turn left");
recog.AddPhrase("","turn right");
recog.AddPhrase("","check my email");

t:loop { 
	recog.Poll(true); 
	echo(recog.result);
},



//////////////////////////////////////////
Urbiscript example 2:

loadModule("URecog");
var recog=URecog.new("InProc",0,0);

recog.LoadGrammar("speech.grxml");

t:loop { 
	recog.Poll(true); 
	echo(recog.result);
},

//////////////////////////////////////////
Urbiscript example 3:

loadModule("URecog");
var recog=URecog.new("InProc",0,0);

recog.LoadGrammar("EX_simple_mtags.grxml");

t:loop { 
	recog.Poll(true); 
	echo(recog.result);
	echo(recog.semantics);
},

