# Timezone

Timezone is an arduino sketch, meant for an esp32 or esp8266 microcontroller connected to a simple 4-digit 7 segment display,  that in its native form, operates as an internet-based clock. 

Besides exposing a normal 12hr clock interface, the sketch also allows for a button peripheral to cycle through different "timezones". While a timezone could be EST or Mountain time, it also can take a looser definition such as "number of days until graduation" or "number of days since sobriety".

When one uses the button to cycle to a non-standard timezone, the clock automatically calculates and displays the relevant number. 

The hope is to give the user the ability to change their frame of reference for time in non-standard, meaningful ways. 
