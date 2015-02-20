# Phoenix Tibia

This is a replacement for the Open Tibia Server and derivatives. The program runs a private server for the game Tibia, in a very modular manner.

That means that you may run each service in a separate machine (but also, you may run them all in the same machine).


## General Details

The server uses the microkernel architecture, meaning that it is a small program that does nothing by itself, but is extensible via several plug-ins and components.

### Plug-ins

By default, the server comes with the following plugins:

  - AccountService (account-databaseserver)
    - Handles the I/O operations related to accounts.
    - This plug-in creates a service that loads all details about an account and some information about the characters belonging to that account
  - HelloWorld (hello-world)
    - This plug-in does nothing - just demonstrate the Plugin API
  - Interserver (interserver)
    - Handles the communication in between all the running services
    - Recommended to be run in a separate instance of the microkernel, with no connection to the internet
  - InterserverClient (interserver-client)
    - Handles the communication with a microkernel with the Interserver plugin
    - Required to be loaded by each microkernel when separating the services
  - LoginService (loginservice)
    - Front-end to the users
    - Handles the first connection, which validades the account and password
    - Uses the AccountService plug-in in order to perform that validation
