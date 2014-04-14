/*
 *
 * Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2 of the License)
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

// This compilation unit can be used with whatever group of tests that need to be run together.
#include <stdio.h>

#include <psconfig.h>

#include <gtest/gtest.h>
// This should be compiled as a standalone app.
CS_IMPLEMENT_APPLICATION


int main(int argc, char** argv)
{
    printf("Running main() from gtest_main.cc\n");

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

