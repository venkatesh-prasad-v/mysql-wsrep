/*
   Copyright (C) 2003-2006 MySQL AB
    All rights reserved. Use is subject to license terms.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is also distributed with certain software (including
   but not limited to OpenSSL) that is licensed under separate terms,
   as designated in a particular file or component or in included license
   documentation.  The authors of MySQL hereby grant you an additional
   permission to link the program and your derivative works with the
   separately licensed software that they have included with MySQL.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include <ndb_global.h>

#include <ArrayList.hpp>
#include <NdbOut.hpp>
#include <NdbTick.h>
#include <NdbMain.h>

#include "arrayListTest.cpp"
#include "arrayPoolTest.cpp"

NDB_COMMAND(al_test, "al_test", "al_test", "al_test", 65535)
{
  NdbMem_Create();
  srand(NdbTick_CurrentMillisecond());

#if 1
  ndbout << endl << endl << "-- Testing basic basic seize/release" << endl;
  ArrayListTest::tryList0(10);

  ndbout << endl << endl << "-- Testing basic seize/release" << endl;
  ArrayListTest::tryList1(1000, 1000);

  ndbout << endl << endl << "-- Testing that seize returns RNIL" 
	 << endl;
  ArrayListTest::tryList1(10, 1000000);
  
  ndbout << endl << endl << "-- Testing access out of array" << endl;
  ArrayListTest::tryList2(1000, 100000, 5);
#endif

#if 1
  ndbout << endl << endl << "-- Testing basic seize/release" << endl;
  ArrayPoolTest::tryPool1(1000, 1000);

  ndbout << endl << endl << "-- Testing that seize returns RNIL" 
	 << endl;
  ArrayPoolTest::tryPool1(10, 1000000);
  
  ndbout << endl << endl << "-- Testing access out of array" << endl;
  ArrayPoolTest::tryPool2(1000, 100000, 5);

  ndbout << endl << endl << "-- Testing releasing none seized element" << endl;
  ArrayPoolTest::tryPool3(1000, 5);
#endif
}

void
ErrorReporter::handleBlockAssert(int line)
{
  ndbout << "ErrorReporter::handleAssert activated - " 
	 << " line= " << line << endl;
  //assert(0);
}
