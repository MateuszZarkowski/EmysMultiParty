*******************************************
*
*   USpeech 
*   Simple module to TTS based on Microsoft Speech SDK
*   author: Jan Kedzierski, Wroclaw University of Technology
*     date: 13.12.2014
*      ver: 2.17
*
*******************************************
*/


Description:
//////////////////////////////////////////
URBI module for speech synthesies based on Microsoft Speech SDK.
The Microsoft Speech SDK consists of a Software Development Kit (SDK), 
a Runtime, and Runtime Languages (language packs that enable speech 
recognition or text-to-speech for a specific language). 

It is based on a COM interface. The Speech Platform includes the 
Microsoft.Speech.VoiceXml namespace to support authoring speech 
applications using industry-standard VoiceXML markup language. 
The Speech Platform Runtime includes a VoiceXML runtime. 

This Module provides visemes synchronisation. You can use viseme 
events to generate mouth animations. Viseme eventing is provided by MS SDK. 
 
More about MSP:
http://msdn.microsoft.com/en-us/library/office/hh361572(v=office.14).aspx
MSP SDK
http://www.microsoft.com/en-us/download/details.aspx?id=27226
MSP Runtime
http://www.microsoft.com/en-us/download/details.aspx?id=27225
MSP Languages
http://www.microsoft.com/en-us/download/details.aspx?id=27224

More about SAPI
http://msdn.microsoft.com/en-us/library/ms723627(v=vs.85).aspx
SDK build in Windows.


XML Tags:
////////////////////////////////////////////
MSP text-to-speech (TTS) extensible markup language (XML) tags fall into several categories.
- Voice state control
- Direct item insertion
- Voice context control
- Voice selection
- Custom Pronunciation
 
Example:
The Volume tag controls the volume of a voice. 
The Volume tag has one required attribute: Level. 
The value of this attribute should be an integer between zero and one hundred. 

<volume level="50">
	This text should be spoken at volume level fifty.
<volume level="100">
	This text should be spoken at volume level one hundred.
</volume>

More examples you can find on this page:
http://msdn.microsoft.com/en-us/library/ms717077(v=vs.85).aspx



Visemes:
//////////////////////////////////////////
SPVISEMES lists the SAPI 5 Viseme set.  This set is based on the 
Disney 13 Visemes. Examples given are for the SAPI 5 English Phoneme set.
    
   // Viseme name       // English examples
                        //------------------
    SP_VISEME_0,        // silence
    SP_VISEME_1,        // ae, ax, ah
    SP_VISEME_2,        // aa
    SP_VISEME_3,        // ao
    SP_VISEME_4,        // ey, eh, uh
    SP_VISEME_5,        // er
    SP_VISEME_6,        // y, iy, ih, ix
    SP_VISEME_7,        // w, uw
    SP_VISEME_8,        // ow
    SP_VISEME_9,        // aw
    SP_VISEME_10,       // oy
    SP_VISEME_11,       // ay
    SP_VISEME_12,       // h
    SP_VISEME_13,       // r
    SP_VISEME_14,       // l
    SP_VISEME_15,       // s, z
    SP_VISEME_16,       // sh, ch, jh, zh
    SP_VISEME_17,       // th, dh
    SP_VISEME_18,       // f, v
    SP_VISEME_19,       // d, t, n
    SP_VISEME_20,       // k, g, ng
    SP_VISEME_21        // p, b, m

Module functions:
//////////////////////////////////////////
USpeech.new(); - initialize TTS
USpeech.Speak("Hello world"); - start speech synthesis (UTF-8),
USpeech.GetVoiceAll(); - returns all available in the system voices,
USpeech.voiceNo; - set available voice,
USpeech.volume; - set speech volume 0..100,
USpeech.rate; - set speech rate -10..10,
USpeech.pitch; - set pitch rate -10..10,
USpeech.visemeTrig; - module set this flag if new viseme occur,
USpeech.visemeId; - current viseme ID,
USpeech.nextVisemeId; - next viseme ID,
USpeech.visemeTime; - current viseme time execution,
USpeech.isSpeaking; - speaking flag, 1 during speech synthesis and 0 when finish,

Urbiscript example:
//////////////////////////////////////////


loadModule("USpeech");
var speech=USpeech.new();

speech.&visemeTrig.notifyChange( closure() {robot.setMouth(speech.visemeId,speech.visemeTime);});
// or just
speech.&visemeTrig.notifyChange( closure() {echo("viseme no. "+speech.visemeId+" exec. time "+speech.visemeTime);});

speech.Speak("Hello world");

speech.voiceNo=1;
speech.volume=100;
speech.rate=-10;
speech.Speak("Hello. My name is robot, robot FLASH.");
