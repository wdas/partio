/*
Copyright Disney Enterprises, Inc. All rights reserved.

This license governs use of the accompanying software. If you use the software, you
accept this license. If you do not accept the license, do not use the software.

1. Definitions
The terms "reproduce," "reproduction," "derivative works," and "distribution" have
the same meaning here as under U.S. copyright law. A "contribution" is the original
software, or any additions or changes to the software. A "contributor" is any person
that distributes its contribution under this license. "Licensed patents" are a
contributor's patent claims that read directly on its contribution.

2. Grant of Rights
(A) Copyright Grant- Subject to the terms of this license, including the license
conditions and limitations in section 3, each contributor grants you a non-exclusive,
worldwide, royalty-free copyright license to reproduce its contribution, prepare
derivative works of its contribution, and distribute its contribution or any derivative
works that you create.
(B) Patent Grant- Subject to the terms of this license, including the license
conditions and limitations in section 3, each contributor grants you a non-exclusive,
worldwide, royalty-free license under its licensed patents to make, have made,
use, sell, offer for sale, import, and/or otherwise dispose of its contribution in the
software or derivative works of the contribution in the software.

3. Conditions and Limitations
(A) No Trademark License- This license does not grant you rights to use any
contributors' name, logo, or trademarks.
(B) If you bring a patent claim against any contributor over patents that you claim
are infringed by the software, your patent license from such contributor to the
software ends automatically.
(C) If you distribute any portion of the software, you must retain all copyright,
patent, trademark, and attribution notices that are present in the software.
(D) If you distribute any portion of the software in source code form, you may do
so only under this license by including a complete copy of this license with your
distribution. If you distribute any portion of the software in compiled or object code
form, you may only do so under a license that complies with this license.
(E) The software is licensed "as-is." You bear the risk of using it. The contributors
give no express warranties, guarantees or conditions. You may have additional
consumer rights under your local laws which this license cannot change.
To the extent permitted under your local laws, the contributors exclude the
implied warranties of merchantability, fitness for a particular purpose and non-
infringement.
*/
#ifndef ROP_TOPARTIO_H
#define ROP_TOPARTIO_H

#include <ROP/ROP_Node.h>

#define STR_PARM(name, vi, t) { evalString(str, name, vi, t); }

class OP_TemplatePair;

class ROP_ToPartio : public ROP_Node
{
public:
    /// Provides access to our parm templates.
    static OP_TemplatePair *getTemplatePair();
    /// Creates an instance of this node.
    static OP_Node *myConstructor(OP_Network *net, const char*name, OP_Operator *op);

protected:
    ROP_ToPartio(OP_Network *net, const char *name, OP_Operator *entry);
    virtual ~ROP_ToPartio();

    /// Called at the beginning of rendering to perform any intialization 
    /// necessary.
    /// @param	nframes	    Number of frames being rendered.
    /// @param	s	    Start time, in seconds.
    /// @param	e	    End time, in seconds.
    /// @return		    True of success, false on failure (aborts the render).
    virtual int startRender(int nframes, fpreal s, fpreal e);

    /// Called once for every frame that is rendered.
    /// @param	time	    The time to render at.
    /// @param	boss	    Interrupt handler.
    /// @return		    Return a status code indicating whether to abort the
    ///			    render, continue, or retry the current frame.
    virtual ROP_RENDER_CODE renderFrame(fpreal time, UT_Interrupt *boss);

    /// Called after the rendering is done to perform any post-rendering steps
    /// required.
    /// @return		    Return a status code indicating whether to abort the
    ///			    render, continue, or retry.
    virtual ROP_RENDER_CODE endRender();

public:
 /// A convenience method to evaluate our custom file parameter.
    void OUTPUT(UT_String &str, float t) { STR_PARM("file",  0, t) }
    void SOPPATH(UT_String &str, float t) { STR_PARM("soppath", 0, t) }

private:
    bool partio_fileSave(const GU_Detail *gdp, const char *fname, float t);

    float myEndTime;
};
#endif
