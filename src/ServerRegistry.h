#ifndef IQMOL_SERVERREGISTRY_H
#define IQMOL_SERVERREGISTRY_H
/*******************************************************************************
       
  Copyright (C) 2011 Andrew Gilbert
           
  This file is part of IQmol, a free molecular visualization program. See
  <http://iqmol.org> for more details.
       
  IQmol is free software: you can redistribute it and/or modify it under the
  terms of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your option) any later
  version.

  IQmol is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
  details.
      
  You should have received a copy of the GNU General Public License along
  with IQmol.  If not, see <http://www.gnu.org/licenses/>.  
   
********************************************************************************/

#include <QStringList>


namespace IQmol {

   class Server;

   /// The ServerRegistry is a singleton that provides a global entry point 
   /// to the Server objects.  The ServerRegistry loads and saves Server 
   /// configuration to the preferences and acts as a pseudo Server factory
   /// via the newLocalServer() and newRemoteServer() functions.
   class ServerRegistry {

      public:
         /// Global access point for the ServerRegistry
         static ServerRegistry& instance();

         /// Serves as constructor for Servers
         Server* newServer();

         /// Returns a list of the names of all the Servers in the ServerRegistry
         QStringList availableServers() const;

		 /// Checks if the Server already exists in the ServerRegistry and, if
		 /// so, returns the correponding pointer.  If not, it returns null.
         Server* get(QString const& serverName) const;

		 /// Removes the Server from the ServerRegistry so that it can no 
		 /// longer be accessed.  Although the Server configuration cannot 
		 /// be recovered, the server is not deleted until the program 
		 /// terminates so existing Processes can still function (this is s
         /// pretty silly thing to want to do though).
         void remove(QString const& serverName);
         void remove(Server*);

		 /// Moves the Server up the list.  This is useful as the first Server
		 /// is considered to be the default.
         void moveUp(QString const& serverName);

		 /// Moves the Server down the list.  This is useful as the first Server
		 /// is considered to be the default.
         void moveDown(QString const& serverName);

         static void saveToPreferences();


      private:
         static ServerRegistry* s_instance;
         static QList<Server*> s_servers;
         static QList<Server*> s_deletedServers;

         static void destroy();
         static int  indexOf(QString const& serverName);
         static void loadFromPreferences();

         // Hide these to prevent multiple instances
         ServerRegistry() { }
         explicit ServerRegistry(ServerRegistry const&) { }
         ~ServerRegistry() { }
   };


} // end namespace IQmol


#endif
