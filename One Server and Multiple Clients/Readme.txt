Client:
./ct
./ct NAME
./ct NAME PORT
./ct NAME IP PORT

Default NAME = Client
Default IP = 127.0.0.1
Default PORT = 1234

Upon connecting to server, client MUST send NAME to server to proceed.
My ./ct does this automatically.

Server:
./sr
./sr PORT
./sr IP PORT

Default IP = 127.0.0.1
Default PORT = 1234

Client now vs. before
The new client sends its name to server immediately upon connection, old does not.
The new client also accepts messages from server like "FULL" for termination.

Type "--exit--" on either server or client, exits the program.