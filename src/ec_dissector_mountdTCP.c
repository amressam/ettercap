/*
    ettercap -- dissector mountd (NFS) -- RPC

    Copyright (C) 2001  ALoR <alor@users.sourceforge.net>, NaGA <crwm@freemail.it>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

    $Id: ec_dissector_mountdTCP.c,v 1.5 2001/12/09 20:24:51 alor Exp $
*/

#include "include/ec_main.h"

#include "include/ec_dissector.h"
#include "include/ec_inet_structures.h"

#define XID_LEN 1024

int XID_MAP[XID_LEN];
int versions[XID_LEN];
char *REM_DIRS[XID_LEN];

FUNC_DISSECTOR(Dissector_mountdTCP);

FUNC_DISSECTOR(Dissector_mountdTCP)
{
   TCP_header *tcp;
   u_char *buf;
   int XID,type;

   tcp = (TCP_header *) data;
   buf = (char *)((int)tcp + tcp->doff * 4);

   if (data_to_ettercap->datalen==0) return (0);

   type    = *(int *)(buf+8);
   XID     = *(int *)(buf+4);

   sprintf(data_to_ettercap->type, "mountd");

   if (ntohs(tcp->source) != SERV_PORT)
   {
      int program,proc,version;

      program = *(int *)(buf + 16);
      proc    = *(int *)(buf + 24);
      version = *(int *)(buf + 20);

      // CALL
      if (type == 0 && ntohl(program) == 100005 && ntohl(proc) == 1 )
      {
         int len, i, cred;

#ifdef DEBUG
   Debug_msg("\tDissector_mountd TCP - CALL");
#endif

         for (i=0; i<XID_LEN; i++)
            if (!XID_MAP[i]) break;

         if (i==XID_LEN) return (0);

         XID_MAP[i] = XID;
         cred = *(int *)(buf + 32);
         cred = ntohl(cred);
         len = *(int *)(buf+44+cred);
         if (ntohl(len)>100)
         {
            XID_MAP[i] = 0;
            return 0;
         }
         REM_DIRS[i] = calloc(1, ntohl(len)+1);
         memcpy(REM_DIRS[i], buf+48+cred, ntohl(len));
         versions[i]=version;
      }
      return (0);
   }

   // REPLY
   if ( ntohl(type) == 1 )
   {
      int i, len,result,offs;
      char *outstr;

#ifdef DEBUG
   Debug_msg("\tDissector_mountd TCP - REPLY");
#endif

      for (i=0; i<XID_LEN; i++)
         if (XID_MAP[i]==XID) break;

      if (i==XID_LEN) return (0);

      XID_MAP[i]=0;

      result = *(int *)(buf+28);
      if (result)
      {
         free(REM_DIRS[i]);
         return (0); //Unautorized
      }

      outstr = data_to_ettercap->info;
      snprintf(outstr, sizeof(data_to_ettercap->info), "NFS SERVER: %s  FHANDLE: %s [", data_to_ettercap->source_ip, REM_DIRS[i]);

      free(REM_DIRS[i]);

      if (ntohl(versions[i])==3)
      {
         len = *(int *)(buf+32);
         len = ntohl(len);
         if (len>64) len=64;
         offs=36;
      }
      else
      {
         len = 32;
         offs= 32;
      }

      for (i=0; i<len-1; i++)
         snprintf(outstr, sizeof(data_to_ettercap->info), "%s%.2x ", outstr, buf[i+offs]);

      snprintf(outstr, sizeof(data_to_ettercap->info), "%s%.2x]\n", outstr, buf[i+offs]);

      sprintf(data_to_ettercap->user, "\n");
      sprintf(data_to_ettercap->pass, "\n");
   }
   return 0;
}

