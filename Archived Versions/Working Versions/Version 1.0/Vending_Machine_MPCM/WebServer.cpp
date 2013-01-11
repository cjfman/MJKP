#define DEBUG_MODE 0

#include "WebServer.h"
#include <Ethernet.h>

Server KingOfPopDebug = Server(169); //Declars server on port 169
Server KingOfPopAdmin = Server(44); //Declars server on port 44
Client Admin = 0;

AdminWebServer::AdminWebServer()
{
  first_login = TRUE;
  authenticated = FALSE;
  wrong = 0;
  pos = 0;
  comstep = 0;
  up_time = 0;
  time_out = 30000;
}

//Public Methods////////////////////////////////////////////////

void AdminWebServer::check(void)
{
  Admin = KingOfPopAdmin.available();
  this->timeOut();
  if (Admin)
  {
    this->upTime();
    String prompt = this->getPrompt();
    this->authenticate(prompt);
    if (!authenticated)
    {
      return;
    }
    String command = prompt.toLowerCase();
    if (command == "quit")
    {
      this->disconnect();
    }
    else if (command == "cancel")
    {
      comstep = 0;
      pos = 0;
      Accounts.cancelSession();
    }
    switch (pos)
    {
    case 0 : 
      this->mainMenu(command); 
      break;
    case 1 : 
      this->addAccount(command); 
      break;
    case 2 : 
      this->listAllAccounts(command);
      break;
    case 3 : 
      this->editAccount(command);
      break;
    case 4 : 
      this->checkBalance(command);
      break;
    case 5 : 
      this->creditAccount(command);
      break;
    case 6 :
      this->chargeAccount(command);
      break;
    }
  }
}

//Private Methods///////////////////////////////////////////////

String AdminWebServer::getPrompt(void)
{
  String prompt;
  while (Admin.available()) 
  {
    char c = Admin.read();
    if (c != '\r' && c != '\n')
    {
      prompt += c;
    }
  }
  return prompt;
}

void AdminWebServer::authenticate(String prompt)
{
  if (DEBUG_MODE)
  {
    Admin.println("");
    Admin.println("DEBUG MODE!");
    authenticated = TRUE;
    return;
  } 
  if (authenticated)
  {
    return;
  }
  if (first_login)
  {
    this->about();
    first_login = FALSE;
    Admin.print("Password: ");
    return;
  }
  else if (prompt == PASSWORD)
  {
    authenticated = TRUE;
    return;
  }
  else if (wrong < 2)
  {
    wrong++;
    Admin.println("");
    Admin.print("Password Incorret. Re-type password: ");
    return;
  }
  else
  {
    Admin.println("");
    Admin.println("");
    Admin.println("Too many incorrect tries. Goodbye.");
    this->disconnect();
    first_login = TRUE;
    wrong = 0;
    return;
  }
}

void AdminWebServer::mainMenu(String command)
{
  if ((comstep == 0) || (command == ""))
  {
    Admin.println("Welcome");
    Admin.println("");
    Admin.println("Options:");
    //Admin.println("> Soda
    Admin.println("> Add Account");
    Admin.println("> Edit Account");
    Admin.println("> List all Accounts");
    Admin.println("> Check Balance");
    Admin.println("> Credit");
    Admin.println("> Charge");
    Admin.println("> Quit");
    Admin.println("");
    Admin.print("Select Option: ");
    comstep++;
    return;
  }
  if (command == "add account")
  {
    pos = 1;
    comstep = 0;
    this->addAccount(command);
    return;
  }
  if (command == "list all accounts")
  {
    pos = 2;
    comstep = 0;
    this->listAllAccounts(command);
    return;
  }
  else if (command == "edit account")
  {
    pos = 3;
    comstep = 0;
    this->editAccount(command);
    return;
  }
  else if (command == "check balance")
  {
    pos = 4;
    comstep = 0;
    this->checkBalance(command);
    return;
  }
  else if (command == "credit")
  {
    pos = 5;
    comstep = 0;
    this->creditAccount(command);
    return;
  }
  else if (command == "charge")
  {
    pos = 6;
    comstep = 0;
    this->chargeAccount(command);
    return;
  }
  else
  {
    Admin.println("");
    Admin.print("Not an opiton. Please choose an option: ");
    return;
  }
}

void AdminWebServer::about(void)
{
  Admin.println("");
  Admin.println("Cashless Payment Device for use with MDB Protocol");
  Admin.println("Designed for use with MIT IDs");
  Admin.println("Copyrighasdft(c) 2011, Charles Franklin <cjfman@mit.edu>");
  Admin.println("");
  Admin.println("This console if for admisistration purposes");
  Admin.println("");
}

void AdminWebServer::disconnect(void)
{
  Admin.println("");
  Admin.println("");
  Admin.println("Good Bye!");
  Admin.println("");
  Admin.println("");
  first_login = TRUE;
  authenticated = FALSE;
  wrong = 0;
  pos = 0;
  comstep = 0;
  up_time = 0;
  time_out = 30000;
  Accounts.closeSession();
  Admin.stop();
}

void AdminWebServer::addAccount(String input)
{
  switch (comstep)
  {
  case 0:
    Admin.println("");
    Admin.print("Name: ");
    comstep++;
    break;
  case 1:
    Admin.println("");
    if (input.length() < 3)
    { 
      Admin.println("Minimum name length is 3 characters.");
      Admin.print("Please re-enter name: ");
      break;
    }
    if (Accounts.getAccountBalance(name) != -1)
    {
      Admin.println("Account name already taken.");
      Admin.print("Please re-enter name: ");
      break;
    }
    name = input;
    Admin.print("ID (type auto for auto-assignment): ");
    comstep++;
    break;
  case 2:
    Admin.println("");
    if (input == "auto")
    {
      String result = Accounts.createAccount(name);
      Admin.println(result);
      comstep = 0;
      pos = 0;
      break;
    }
    ID = this->convertStringToLong(input);
    if (ID == -1)
    {
      Admin.println("Please either all numerals, or 'auto'");
      Admin.print("Please re-enter ID: ");
      return;
    }
    if (input.length() < 6)
    { 
      Admin.println("Minimum id length is 6 characters.");
      Admin.print("Please re-enter ID: ");
      break;
    }
    if (Accounts.getAccountIDBalance(ID) != -1)
    {
      Admin.println("Account ID already taken.");
      Admin.print("Please re-enter ID: ");
      break;
    }
    String result = Accounts.createAccount(name, ID);
    Admin.println(result);
    comstep = 0;
    pos = 0;
    break;
  }
}

void AdminWebServer::listAllAccounts(String input)
{
  Admin.println("");
  Admin.println("All of Micheal Jackson's Fans");
  Admin.println("");
  Admin.println(Accounts.allAccounts());
  pos = 0;
}

void AdminWebServer::editAccount(String command)
{
  static int NI = 0;
  Admin.println("");
  switch (comstep)
  {
  case 0:
    {
      Admin.println("Find Account Using:\n>Name\n>ID");
      Admin.println("");
      Admin.print("Please choose one: ");
      comstep++;
      return;
    }
  case 1:
    if (command == "name")
    {
      Admin.print("Name: ");
      NI = 0;
      comstep++;
      return;
    }
    else if (command == "id")
    {
      Admin.print("ID: ");
      NI = 1;
      comstep++;
      return;
    }
    else
    {
      Admin.println("");
      Admin.print("Not an opiton. Please choose an option: ");
      return;
    }
    comstep++;
    return;
  case 2:
    switch(NI)
    {
    case 0:
      name = command;
      ID = Accounts.getAccountID(command);
      balance = Accounts.getAccountBalance(command);
      break;
    case 1:
      long id = this->convertStringToLong(command);
      if (id == -1)
      {
        Admin.println("Please only enter numerals");
        Admin.print("Please re-enter ID: ");
        return;
      }
      ID = id;
      name = Accounts.getAccountName(id);
      balance = Accounts.getAccountIDBalance(id);
      break;
    }
    if (balance == -1)
    {
      Admin.println("");
      Admin.println("Account does not exist!");
      pos = 0;
      comstep = 0;
      return;
    }
    Admin.println("");
    Admin.print("Name: ");
    Admin.print(name);
    Admin.print("    ID: ");
    Admin.println(ID);
    Admin.println("");
    Admin.println("Account Options:");
    Admin.println("> Change Name");
    Admin.println("> Change ID");
    Admin.println("> Check Balance");
    Admin.println("> Credit Account");
    Admin.println("> Charge Account");
    Admin.println("> Purchase Count");
    Admin.println("> Delete Account");
    Admin.println("> Exit");
    Admin.println("");
    Admin.print("Select an option: ");
    comstep++;
    return;
  case 3:
    if (command == "change name")
    {
      Admin.println("");
      Admin.print("New Name: ");
      NI = 0;
    }
    else if (command == "change id")
    {
      Admin.println("");
      Admin.print("New ID: ");
      NI = 1;
    }
    else if (command == "check balance")
    {
      Admin.println("");
      Admin.println("");
      Admin.print("Balance: ");
      Admin.println(Accounts.getAccountBalance(name));
      comstep--;
      NI = 3;
      return;
    }
    else if (command == "credit account")
    {
      Admin.println("");
      Admin.println("Dolar values only. No cents");
      Admin.print("Amount: ");
      NI = 2;
    }
    else if (command == "charge account")
    {
      Admin.println("");
      Admin.println("Dolar values only. No cents");
      Admin.print("Amount: ");
      NI = 3;
    }
    else if (command == "purchase count")
    {
      Admin.println("");
      Admin.println(Accounts.count(name));
      NI = 3;
      comstep--;
      return;
    }
    else if (command == "delete account")
    {
      Admin.println("");
      Admin.println("Are you sure? This action cannot be undone");
      Admin.println("All funds will be lost");
      Admin.print("(YES/NO): ");
      NI = 4;
    }
    else if (command == "exit")
    {
      Admin.println("");
      Admin.println("Account Info Updated");
      Accounts.save();
      comstep = 0;
      pos = 0;
      return;
    }
    else if (command == "")
    {
      comstep--;
      return;
    }
    else
    {
      Admin.println("");
      Admin.print("Not an option. Please select a valid option: ");
      return;
    }
    comstep++;
    return;
  case 4:
    switch(NI)
    {
    case 0:
      if (command.length() < 3)
      { 
        Admin.println("");
        Admin.println("Minimum name length is 3 characters.");
        Admin.print("Please re-enter name: ");
        return;
      }
      if (Accounts.getAccountBalance(command) != -1)
      {
        Admin.println("");
        Admin.println("Account name already exists");
        Admin.print("Please re-enter name: ");
        return;
      }
      name = Accounts.changeAccountName(name, command);
      comstep--;
      NI = 3;
      return;
    case 1:
    {
      unsigned long id = this->convertStringToLong(command);
      if (id == -1)
      {
        Admin.println("Please either all numerals, or 'auto'");
        Admin.print("Please re-enter ID: ");
        return;
      }
      if (command.length() < 6)
      { 
        Admin.println("Minimum id length is 6 characters.");
        Admin.print("Please re-enter ID: ");
        return;
      }
      ID = id;
      Accounts.changeAccountID(name, ID);
      comstep--;
      NI = 3;
      return;
    }
    case 2:
    {
      long credit = this->convertStringToLong(command);
      Admin.println("");
      if (credit == -1)
      {
        Admin.println("Please either all numerals");
        Admin.print("Please re-enter $ amount: ");
        return;
      }
      Admin.print("Balance: ");
      Admin.println(Accounts.creditAccount(name, credit));
      NI = 3;
      comstep--;
      return;
    }
    case 3:
    {
      long charge = this->convertStringToLong(command);
      Admin.println("");
      if (charge == -1)
      {
        Admin.println("Please either all numerals");
        Admin.print("Please re-enter $ amount: ");
        return;
      }
      Admin.print("Balance: ");
      Admin.println(Accounts.creditAccount(name, -1 * charge));
      NI = 3;
      comstep--;
      return;
    }
    case 4:
      if (command == "yes")
      {
        Accounts.deleteAccount(name);
        Accounts.save();
        Admin.println("");
        Admin.println("Account Permanently Deleted");
        comstep = 0;
        pos = 0;
      }
      else if (command == "no")
      {
        NI = 3;
        comstep--;
      }
      else
      {
        Admin.println("");
        Admin.println("How hard is it to type either yes or no?");
        Admin.print("Let's try this again. [YES/NO]: ");
      }
    }
  }
}



void AdminWebServer::checkBalance(String command)
{
  static int NI = 0;
  Admin.println("");
  switch (comstep)
  {
  case 0:
    {
      Admin.println("Find Account Using:\n>Name\n>ID");
      Admin.println("");
      Admin.print("Please choose one: ");
      comstep++;
      return;
    }
  case 1:
    if (command == "name")
    {
      Admin.print("Name: ");
      NI = 0;
      comstep++;
      return;
    }
    else if (command == "id")
    {
      Admin.print("ID: ");
      NI = 1;
      comstep++;
      return;
    }
    else
    {
      Admin.println("");
      Admin.print("Not an opiton. Please choose an option: ");
      return;
    }
  case 2:
    switch(NI)
    {
    case 0:
      balance = Accounts.getAccountBalance(command);
      break;
    case 1:
      long id = this->convertStringToLong(command);
      if (id == -1)
      {
        Admin.println("Please only enter numerals");
        Admin.print("Please re-enter ID: ");
        return;
      }
      balance = Accounts.getAccountIDBalance(id);
      break;
    }
    if (balance == -1)
    {
      Admin.println("");
      Admin.println("Account does not exist!");
    }
    else
    {
      Admin.print("Balance: ");
      Admin.println(balance);
    }
    pos = 0;
    comstep = 0;
    return;
  }
}

void AdminWebServer::creditAccount(String command)
{
  static int NI = 0;
  Admin.println("");
  switch (comstep)
  {
  case 0:
    {
      Admin.println("Find Account Using:\n>Name\n>ID");
      Admin.println("");
      Admin.print("Please choose one: ");
      comstep++;
      return;
    }
  case 1:
    if (command == "name")
    {
      Admin.print("Name: ");
      NI = 0;
      comstep++;
      return;
    }
    else if (command == "id")
    {
      Admin.print("ID: ");
      NI = 1;
      comstep++;
      return;
    }
    else
    {
      Admin.println("");
      Admin.print("Not an opiton. Please choose an option: ");
      return;
    }
  case 2:
    switch(NI)
    {
    case 0:
      name = command;
      break;
    case 1:
      ID = this->convertStringToLong(command);
      if (ID == -1)
      {
        Admin.println("Invalid ID. Please only enter numerals");
        Admin.print("Please re-enter ID: ");
        return;
      }
    }
    Admin.print("Enter amount to credit (no cents): ");
    comstep++;
    return;
  case 3:
    balance = this->convertStringToLong(command);
    if (balance == -1)
    {
      Admin.println("Please only enter numerals");
      Admin.print("Please re-enter amount to credit: ");
      return;
    }
    switch(NI)
    {
    case 0:
      balance = Accounts.creditAccount(name, balance);
      break;
    case 1:
      balance = Accounts.creditAccount(ID, balance);
      break;
    }
    if (balance == -1)
    {
      Admin.println("");
      Admin.println("Account does not exist!");
    }
    else
    {
      Admin.print("Balance: ");
      Admin.println(balance);
    }
    pos = 0;
    comstep = 0;
    return;
  }
}

void AdminWebServer::chargeAccount(String command)
{
  static int NI = 0;
  Admin.println("");
  switch (comstep)
  {
  case 0:
    {
      Admin.println("Find Account Using:\n>Name\n>ID");
      Admin.println("");
      Admin.print("Please choose one: ");
      comstep++;
      return;
    }
  case 1:
    if (command == "name")
    {
      Admin.print("Name: ");
      NI = 0;
      comstep++;
      return;
    }
    else if (command == "id")
    {
      Admin.print("ID: ");
      NI = 1;
      comstep++;
      return;
    }
    else
    {
      Admin.println("");
      Admin.print("Not an opiton. Please choose an option: ");
      return;
    }
  case 2:
    switch(NI)
    {
    case 0:
      name = command;
      break;
    case 1:
      ID = this->convertStringToLong(command);
      if (ID == -1)
      {
        Admin.println("Invalid ID. Please only enter numerals");
        Admin.print("Please re-enter ID: ");
        return;
      }
    }
    Admin.print("Enter amount to credit (no cents): ");
    comstep++;
    return;
  case 3:
    balance = this->convertStringToLong(command);
    if (balance == -1)
    {
      Admin.println("Please only enter numerals");
      Admin.print("Please re-enter amount to credit: ");
      return;
    }
    balance = balance * -1;
    switch(NI)
    {
    case 0:
      balance = Accounts.creditAccount(name, balance);
      break;
    case 1:
      balance = Accounts.creditAccount(ID, balance);
      break;
    }
    if (balance == -1)
    {
      Admin.println("");
      Admin.println("Account does not exist!");
    }
    else
    {
      Admin.print("Balance: ");
      Admin.println(balance);
    }
    pos = 0;
    comstep = 0;
    return;
  }
}

void AdminWebServer::upTime(void)
{
  if (up_time == 0)
  {
    start_time = millis();
    up_time++;
    return;
  }
  time_out = up_time + 30000;
}

void AdminWebServer::timeOut(void)
{
  if (up_time == 0)
  {
    return;
  }
  up_time = millis() - start_time;
  if (up_time >= time_out)
  {
    Admin.println("");
    Admin.println("");
    Admin.println("Session Timed Out!");
    if (Admin)
    {
      this->disconnect();
    }
  }
}

long AdminWebServer::convertStringToLong(String string)
{
  int i;
  long result = 0;
  for (i = 0; i < string.length(); i++)
  {
    if (string[i] == '0')
    {
      result = result*10 + 0;
    }
    else if (string[i] == '1')
    {
      result = result*10 + 1;
    }
    else if (string[i] == '2')
    {
      result = result*10 + 2;
    }
    else if (string[i] == '3')
    {
      result = result*10 + 3;
    }
    else if (string[i] == '4')
    {
      result = result*10 + 4;
    }
    else if (string[i] == '5')
    {
      result = result*10 + 5;
    }
    else if (string[i] == '6')
    {
      result = result*10 + 6;
    }
    else if (string[i] == '7')
    {
      result = result*10 + 7;
    }
    else if (string[i] == '8')
    {
      result = result*10 + 8;
    }
    else if (string[i] == '9')
    {
      result = result*10 + 9;
    }
    else
    {
      return -1;
    }
  }
  return result;
}

AdminWebServer Administrator;


//Other Functions/////////////////////////////////////////////////////////

void debugCheck(void)
{
  static int first_debug = TRUE;
  static int i = 1;
  Client DebugConsole = KingOfPopDebug.available();
  if (DebugConsole)
  {
    String prompt;
    while (DebugConsole.available())
    {
      char c = DebugConsole.read();
      if (c != '\r' && c != '\n')
      {
        prompt += c;
      }
      if (c == '\n')
      {
        if (first_debug || prompt == "about")
        {
          DebugConsole.println("");
          DebugConsole.println("Cashless Payment Device for use with MDB Protocol");
          DebugConsole.println("Designed for use with MIT IDs");
          DebugConsole.println("Copyrighasdft(c) 2011, Charles Franklin <cjfman@mit.edu>");
          first_debug = FALSE;
        }
      }
    }
  }
}

void debug(String info)
{
  KingOfPopDebug.println(info);
}

void debugHex(int data)
{
  KingOfPopDebug.println(data, HEX);
}


