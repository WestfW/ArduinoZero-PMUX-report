# ArduinoZero-PMUX-report
Generate a user-readable report of the current state of the pins (what they're doing) on Arduino Zero and other SAMD21 based systems

In the forum discussion here: http://forum.arduino.cc/index.php?topic=403674.msg2776323 the question is asked
"How can I tell how the Arduino core has configured the various multi-purpose pins of the SAMD21 chip?"

This is a good question, and one that CAN be answered without too much trouble, since the SAMD21 controls the pins
from a central "Pin Multiplexing" datastructure.  Each pin has a bit saying whether it is just GPIO or "something else",
and a separate field describing the "something else."  It's just a SMOP to decode this information.  (and invitingly,
while a full decode is possible, even a relatively minor effort can yield useful information.)
