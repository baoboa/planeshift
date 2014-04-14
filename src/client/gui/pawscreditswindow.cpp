/*
* pawscreditswindow.cpp - Author: Christian Svensson
*
* Copyright (C) 2004 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
//////////////////////////////////////////////////////////////////////

#include <psconfig.h>
#include <ctype.h>
// CS INCLUDES
#include <csgeom/vector3.h>
#include <csutil/xmltiny.h>
#include <iutil/vfs.h>
#include <iutil/objreg.h>
#include <csqint.h>
// COMMON INCLUDES

// CLIENT INCLUDES
#include "globals.h"

// PAWS INCLUDES
#include "paws/pawsmanager.h"
#include "paws/pawstextbox.h"
#include "paws/pawsbutton.h"
#include "paws/pawsprefmanager.h"

//GUI INCLUDES
#include "gui/pawsloginwindow.h"

// PS include
#include "pawscreditswindow.h"
#include "util/psxmlparser.h"
#include "util/localization.h"


//-------------------------------------------------------------------
void pawsCreditsWindow::Show()
{
    pawsWidget::Show();
    pixelAt = 0;
    startPos = -(int)numLines + 1;
}

bool pawsCreditsWindow::PostSetup()
{
    char endOfLine[2] = {10, 0};

    psengine->GetSoundManager()->LoadActiveSector("main");

    font = PawsManager::GetSingleton().GetPrefs()->GetDefaultFont();


    csString filename = PawsManager::GetSingleton().GetLocalization()->FindLocalizedFile("credits_config.xml");
    if (!psengine->GetVFS()->Exists(filename.GetData()))
    {
        Error2( "Could not find XML: %s",filename.GetData());
        return false;
    }

    csRef<iDocument> doc = ParseFile(psengine->GetObjectRegistry(),filename.GetData());
    if (!doc)
    {
        Error2("Parse error in %s", filename.GetData());
        return false;
    }
    csRef<iDocumentNode> root = doc->GetRoot();
    if (!root)
    {
        Error2("No XML root in %s", filename.GetData());
        return false;
    }

    csRef<iDocumentNode> topNode = root->GetNode("credits_config");
    if (!topNode)
    {
        Error2("Missing <credits_config> tag in %s", filename.GetData());
        return false;
    }

    csRef<iDocumentNodeIterator> iter = topNode->GetNodes();

    //Loop through the XML
    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> node = iter->Next();
        //Do you wanna set the color?
        if (!strcmp(node->GetValue(),"color"))
        {
            //Thought so.. Now which? Standard?
            if (!strcmp(node->GetAttributeValue("name"),"standard"))
            {
                standardR = node->GetAttributeValueAsInt("r");
                standardG = node->GetAttributeValueAsInt("g");
                standardB = node->GetAttributeValueAsInt("b");
            }
            //no? @?
            else if (!strcmp(node->GetAttributeValue("name"),"@"))
            {
                atR = node->GetAttributeValueAsInt("r");
                atG = node->GetAttributeValueAsInt("g");
                atB = node->GetAttributeValueAsInt("b");
            }
            //not that either? &!?
            else if (!strcmp(node->GetAttributeValue("name"),"&"))
            {
                andR = node->GetAttributeValueAsInt("r");
                andG = node->GetAttributeValueAsInt("g");
                andB = node->GetAttributeValueAsInt("b");
            }
        }
        //No? Maybe area size then..
        else if (!strcmp(node->GetValue(),"area"))
        {
            //Yes!
            width = GetActualWidth(node->GetAttributeValueAsInt("width"));
            height = GetActualHeight(node->GetAttributeValueAsInt("height"));
            x = GetActualWidth(node->GetAttributeValueAsInt("x"));
            y = GetActualHeight(node->GetAttributeValueAsInt("y"));
            speed = node->GetAttributeValueAsInt("speed");
            loop = node->GetAttributeValueAsBool("loop");
            box = node->GetAttributeValueAsBool("boxed");
        }
        else if (!strcmp(node->GetValue(),"end"))
        {
            endMsg = node->GetAttributeValue("msg");
            endColor = psengine->GetG2D()->FindRGB(
                node->GetAttributeValueAsInt("r"),
                node->GetAttributeValueAsInt("g"),
                node->GetAttributeValueAsInt("b"));
        }

    }

    csRef<iFile> file = psengine->GetVFS()->Open("/this/docs/credit.txt",VFS_FILE_READ); 
    if (!file) 
    {
        Error1 ("Couldn't open credit.txt!");
        return false;
    }

    // read the raw, unformatted lines from the credits, split into words.
    char buf[512];
    size_t readsize;

    do
    {
        readsize = file->Read(buf, 512);
        mainstring.Append(buf, readsize);
    } while (readsize == 512);

    size_t cur;
    const char *dat = mainstring.GetData();

    // Convert the text from DOS format to Unix format :P
    for (cur=0; cur < mainstring.Length(); cur++)
        if (mainstring[cur] == 13)
            mainstring[cur] = 32;

    cur=0;
    while (cur<mainstring.Length() )
    {

        if (mainstring.GetAt(cur) == '@')
        {
            styles.Push(1);
            cur++; // skip formatting character
        }
        else if (mainstring.GetAt(cur) == '&')
        {
            styles.Push(2);
            cur++; // skip formatting character
        }
        else
            styles.Push(0);

        size_t len = strcspn (dat+cur, endOfLine);
        unsigned int maxlen;

        maxlen = font->GetLength(dat+cur, width );

        // only break line after words
        if (maxlen < len)
        {
            len = maxlen;

            // go back to last complete word
            while (!isspace(*(dat+cur+len))) 
            {
                if (--len == 0)
                {
                    len=maxlen + 1;
                    break;
                }
            }
        }

        //Add a null to the string to make the constuctor some lines down know were to stop
        if (cur+len < mainstring.Length())
            mainstring[cur+len] = 0;

        csString newstr = dat+cur;

        credits.Push(newstr);
        cur += len+1;
    }

    int w;
    font->GetMaxSize (w, textHeight);

    numLines = height / textHeight;
    textYofs = y + (int)(height - numLines * textHeight) / 2;

    scrollRate = speed / textHeight;

    startPos = -(int)numLines + 1;
    pixelAt = 0;

    initialTime = csGetTicks();

    return true;

}


//---------------------------------------------------------------------------

void pawsCreditsWindow::Hide()
{
    pawsLoginWindow* loginWnd = (pawsLoginWindow*)PawsManager::GetSingleton().FindWidget("LoginWindow");
    if (!loginWnd)
    {
        Error1("Couldn't find Login window!\n");
        return;
    }
    pawsWidget::Hide();
    loginWnd->Show();

    psengine->GetSoundManager()->LoadActiveSector("main");
}

void pawsCreditsWindow::Draw()
{
    pawsWidget::Draw();
    ClipToParent(false);

    if ( csGetTicks() - initialTime >= scrollRate )
    {
        initialTime = csGetTicks();
        if ( ++pixelAt == textHeight )
        {
            pixelAt = 0;
            startPos++;
        }
    }

    iGraphics2D *g2d = psengine->GetG2D();

    if (box)
        g2d->DrawBox(x,y,width,height,g2d->FindRGB(0,0,0));

    if ( startPos >=(int) credits.GetSize())
    {
        if (loop)
        {
            startPos = 0;
        }
        else
        {
            int sizew = 0,sizeh=0;
            font->GetDimensions(endMsg,sizew,sizeh);

            g2d->Write(font,
                x+(width/2)-(sizew/2),
                y+(height/2)-(sizeh/2), 
                endColor, -1, 
                endMsg );
        }
    }
    else 
    {
        for (int i = 0; i < (int)numLines && i+startPos < (int)credits.GetSize(); i++ )
        {

            int c = -1;
            int cR, cG, cB;

            if (i+startPos >= 0)
            {
                csString line = credits.Get(i+startPos);

                //Set color coding
                if (styles[i+startPos] == 1)
                {
                    cR = atR;
                    cG = atG;
                    cB = atB;
                }
                else if (styles[i+startPos] == 2)
                {
                    cR = andR;
                    cG = andG;
                    cB = andB;
                }
                else
                {
                    cR = standardR;
                    cG = standardG;
                    cB = standardB;
                }

                float alpha = 1.0f;

                if (i == 0)
                {
                    alpha = 1.0f - ((float)pixelAt / (float)textHeight);
                } 
                else if ((uint)i == numLines - 1)
                {
                    alpha = (float)pixelAt / (float)textHeight;
                }

                int black = g2d->FindRGB(0, 0, 0, csQint (alpha * 255.99f));

                g2d->Write(font,
                    x+2,
                    textYofs+i*textHeight-pixelAt+2,
                    black, -1,
                    line.GetData() );

                c = g2d->FindRGB(cR, cG, cB, csQint (alpha * 255.99f));

                //Write the nice row on the screen
                g2d->Write(font,
                    x,
                    textYofs+i*textHeight-pixelAt, 
                    c, -1, 
                    line.GetData() );
            }
        }
    }
}

bool pawsCreditsWindow::OnButtonPressed(int /*button*/, int /*keyModifier*/, pawsWidget* widget)
{
    if (widget->GetID() == 100) //Ok button
    {
        pawsWidget* wdg = PawsManager::GetSingleton().FindWidget("LoginWindow");
        if (!wdg)
        {
            Error1("Could not find Login window!");
            return false;
        }
        wdg->Show();
        this->Hide();
        return true;
    }

    return true;

}

