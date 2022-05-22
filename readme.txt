This is conio, a functional replacement for the IBM model
029 card punch/reader. This has a submit option to send
a card deck to a hercules IBM system 370 emulation.

To build, type:
make

To install, type:
sudo make install

The binary will be in /usr/local/bin

This was written for Linux. It should work for any *nix
system.


To use, type:
conio

You will see an into message and a column ruler (for FORTRAN, Cobol, etc).
Start typing whatever jcl code, source code etc. Maximum line length is
80 columns. The number of cards is limited only by your available
memory/virtual memory. Internally, each card (line) is 80 columns. When 
submitted to the mainframe, or saved to a text file, the line length will
only be as long as the actual data. NULLS are used after the final \n 
on the card.

Commands are preceeded by a period '.' and must start at position 0 of the line.
Available commands are:

.quit             Exit.
.help             Show this list.
.save [filename]  Save the card deck to a named file.
.load [filename]  Load an external file to the card deck.
.submit           Send cards to tcp-based server/device.
.repl [card #]    Replace an existing card. You will be prompted for new.
.new              Clear existing card deck.
.list             Show numbered list of cards.
.del              Delete the last card in the deck.
.ruler            Display column numbers.
.telnet [address][port]   Telnet to supplied address/port.

Typing .submit opens a network connection. You will be presented with
an IP address that you must change in one of the #defines at the files 
beginning. You can hit <enter> to use this address or type in a new
IPv4 address.

If you type in a new address, that address will be used as the default
until the program is exited. The original default will be used the next
time the program is started.

This IP address is NOT used in the telnet application. You must type
it in (along with the port) at each invocation.





