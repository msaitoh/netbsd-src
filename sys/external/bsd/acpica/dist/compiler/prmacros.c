/******************************************************************************
 *
 * Module Name: prmacros - Preprocessor #define macro support
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2023, Intel Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#include "aslcompiler.h"

#define _COMPONENT          ASL_PREPROCESSOR
        ACPI_MODULE_NAME    ("prmacros")


/*******************************************************************************
 *
 * FUNCTION:    PrDumpPredefinedNames
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Dump the list of #defines. Used as the preprocessor starts, to
 *              display the names that were defined on the command line.
 *              Debug information only.
 *
 ******************************************************************************/

void
PrDumpPredefinedNames (
    void)
{
    PR_DEFINE_INFO          *DefineInfo;


    DefineInfo = AslGbl_DefineList;
    while (DefineInfo)
    {
        DbgPrint (ASL_DEBUG_OUTPUT, PR_PREFIX_ID
            "Predefined #define: %s->%s\n",
            0, DefineInfo->Identifier, DefineInfo->Replacement);

        DefineInfo = DefineInfo->Next;
    }
}


/*******************************************************************************
 *
 * FUNCTION:    PrAddDefine
 *
 * PARAMETERS:  Identifier          - Name to be replaced
 *              Replacement         - Replacement for Identifier
 *              Persist             - Keep define across multiple compiles?
 *
 * RETURN:      A new define_info struct. NULL on error.
 *
 * DESCRIPTION: Add a new #define to the global list
 *
 ******************************************************************************/

PR_DEFINE_INFO *
PrAddDefine (
    char                    *Identifier,
    char                    *Replacement,
    BOOLEAN                 Persist)
{
    char                    *IdentifierString;
    char                    *ReplacementString;
    PR_DEFINE_INFO          *DefineInfo;


    if (!Replacement)
    {
        Replacement = "";
    }

    /* Check for already-defined first */

    DefineInfo = PrMatchDefine (Identifier);
    if (DefineInfo)
    {
        DbgPrint (ASL_DEBUG_OUTPUT, PR_PREFIX_ID
            "#define: name already exists: %s\n",
            AslGbl_CurrentLineNumber, Identifier);

        /*
         * Name already exists. This is only an error if the target name
         * is different.
         */
        if (strcmp (Replacement, DefineInfo->Replacement))
        {
            PrError (ASL_ERROR, ASL_MSG_EXISTING_NAME,
                THIS_TOKEN_OFFSET (Identifier));

            return (NULL);
        }

        return (DefineInfo);
    }

    /* Copy input strings */

    IdentifierString = UtLocalCalloc (strlen (Identifier) + 1);
    strcpy (IdentifierString, Identifier);

    ReplacementString = UtLocalCalloc (strlen (Replacement) + 1);
    strcpy (ReplacementString, Replacement);

    /* Init and link new define info struct */

    DefineInfo = UtLocalCalloc (sizeof (PR_DEFINE_INFO));
    DefineInfo->Replacement = ReplacementString;
    DefineInfo->Identifier = IdentifierString;
    DefineInfo->Persist = Persist;

    if (AslGbl_DefineList)
    {
        AslGbl_DefineList->Previous = DefineInfo;
    }

    DefineInfo->Next = AslGbl_DefineList;
    AslGbl_DefineList = DefineInfo;
    return (DefineInfo);
}


/*******************************************************************************
 *
 * FUNCTION:    PrRemoveDefine
 *
 * PARAMETERS:  DefineName          - Name of define to be removed
 *
 * RETURN:      None
 *
 * DESCRIPTION: Implements #undef. Remove a #define if found in the global
 *              list. No error if the target of the #undef does not exist,
 *              as per the C #undef definition.
 *
 ******************************************************************************/

void
PrRemoveDefine (
    char                    *DefineName)
{
    PR_DEFINE_INFO          *DefineInfo;


    /* Match name and delete the node */

    DefineInfo = AslGbl_DefineList;
    while (DefineInfo)
    {
        if (!strcmp (DefineName, DefineInfo->Identifier))
        {
            /* Remove from linked list */

            if (DefineInfo->Previous)
            {
                (DefineInfo->Previous)->Next = DefineInfo->Next;
            }
            else
            {
                AslGbl_DefineList = DefineInfo->Next;
            }

            if (DefineInfo->Next)
            {
                (DefineInfo->Next)->Previous = DefineInfo->Previous;
            }

            free (DefineInfo);
            return;
        }

        DefineInfo = DefineInfo->Next;
    }

    /*
     * Name was not found. By definition of #undef, this is not
     * an error, however.
     */
    DbgPrint (ASL_DEBUG_OUTPUT, PR_PREFIX_ID
        "#undef: could not find %s\n",
        AslGbl_CurrentLineNumber, DefineName);
}


/*******************************************************************************
 *
 * FUNCTION:    PrMatchDefine
 *
 * PARAMETERS:  MatchString         - Name associated with the #define
 *
 * RETURN:      Matched string if found. NULL otherwise.
 *
 * DESCRIPTION: Find a name in global #define list
 *
 ******************************************************************************/

PR_DEFINE_INFO *
PrMatchDefine (
    char                    *MatchString)
{
    PR_DEFINE_INFO          *DefineInfo;


    DefineInfo = AslGbl_DefineList;
    while (DefineInfo)
    {
        if (!strcmp (MatchString, DefineInfo->Identifier))
        {
            return (DefineInfo);
        }

        DefineInfo = DefineInfo->Next;
    }

    return (NULL);
}


/*******************************************************************************
 *
 * FUNCTION:    PrAddMacro
 *
 * PARAMETERS:  Name                - Start of the macro definition
 *              Next                - "Next" buffer from GetNextToken
 *
 * RETURN:      None
 *
 * DESCRIPTION: Add a new macro to the list of #defines. Handles argument
 *              processing.
 *
 ******************************************************************************/

void
PrAddMacro (
    char                    *Name,
    char                    **Next)
{
    char                    *Token = NULL;
    ACPI_SIZE               TokenOffset;
    ACPI_SIZE               MacroBodyOffset;
    PR_DEFINE_INFO          *DefineInfo;
    PR_MACRO_ARG            *Args;
    char                    *Body;
    char                    *BodyInSource;
    UINT32                  i;
    UINT16                  UseCount = 0;
    UINT16                  ArgCount = 0;
    UINT32                  Depth = 1;
    /*UINT32                  Depth = 1;*/
    UINT32                  EndOfArgList;
    char                    BufferChar;

    /* Find the end of the arguments list */

    TokenOffset = Name - AslGbl_MainTokenBuffer + strlen (Name) + 1;
    while (1)
    {
        BufferChar = AslGbl_CurrentLineBuffer[TokenOffset];
        if (BufferChar == '(')
        {
            Depth++;
        }
        else if (BufferChar == ')')
        {
            Depth--;
        }
        else if (BufferChar == 0)
        {
            PrError (ASL_ERROR, ASL_MSG_MACRO_SYNTAX, TokenOffset);
            return;
        }

        if (Depth == 0)
        {
            /* Found arg list end */

            EndOfArgList = TokenOffset;
            break;
        }

        TokenOffset++;
    }

    /* At this point, we know that we have a reasonable argument list */

    Args = UtLocalCalloc (sizeof (PR_MACRO_ARG) * PR_MAX_MACRO_ARGS);

    /* Get the macro argument names */

    for (i = 0; i < PR_MAX_MACRO_ARGS; i++)
    {
        Token = PrGetNextToken (NULL, PR_MACRO_SEPARATORS, Next);

        if (!Token)
        {
            /* This is the case for a NULL macro body */

            BodyInSource = "";
            goto AddMacroToList;
        }

        /* Don't go beyond the argument list */

        TokenOffset = Token - AslGbl_MainTokenBuffer + strlen (Token);
        if (TokenOffset > EndOfArgList)
        {
            break;
        }

        DbgPrint (ASL_DEBUG_OUTPUT, PR_PREFIX_ID
            "Macro param: %s\n",
            AslGbl_CurrentLineNumber, Token);

        Args[i].Name = UtLocalCalloc (strlen (Token) + 1);
        strcpy (Args[i].Name, Token);

        Args[i].UseCount = 0;
        ArgCount++;
        if (ArgCount >= PR_MAX_MACRO_ARGS)
        {
            PrError (ASL_ERROR, ASL_MSG_TOO_MANY_ARGUMENTS, TokenOffset);
            goto ErrorExit;
        }
    }

    /* Get the macro body. Token now points to start of body */

    MacroBodyOffset = Token - AslGbl_MainTokenBuffer;

    /* Match each method arg in the macro body for later use */

    while (Token)
    {
        /* Search the macro arg list for matching arg */

        for (i = 0; ((i < PR_MAX_MACRO_ARGS) && Args[i].Name); i++)
        {
            /*
             * Save argument offset within macro body. This is the mechanism
             * used to expand the macro upon invocation.
             *
             * Handles multiple instances of the same argument
             */
            if (!strcmp (Token, Args[i].Name))
            {
                UseCount = Args[i].UseCount;

                Args[i].Offset[UseCount] =
                    (Token - AslGbl_MainTokenBuffer) - MacroBodyOffset;


                DbgPrint (ASL_DEBUG_OUTPUT, PR_PREFIX_ID
                    "Macro Arg #%u: %s UseCount %u Offset %u\n",
                    AslGbl_CurrentLineNumber, i, Token,
                    UseCount+1, Args[i].Offset[UseCount]);

                Args[i].UseCount++;

                if (Args[i].UseCount >= PR_MAX_ARG_INSTANCES)
                {
                    PrError (ASL_ERROR, ASL_MSG_TOO_MANY_ARGUMENTS,
                        THIS_TOKEN_OFFSET (Token));

                    goto ErrorExit;
                }
                break;
            }
        }

        Token = PrGetNextToken (NULL, PR_MACRO_SEPARATORS, Next);
    }

    BodyInSource = &AslGbl_CurrentLineBuffer[MacroBodyOffset];


AddMacroToList:

    /* Check if name is already defined first */

    DefineInfo = PrMatchDefine (Name);
    if (DefineInfo)
    {
        DbgPrint (ASL_DEBUG_OUTPUT, PR_PREFIX_ID
            "#define: macro name already exists: %s\n",
            AslGbl_CurrentLineNumber, Name);

        /* Error only if not exactly the same macro */

        if (strcmp (DefineInfo->Body, BodyInSource) ||
            (DefineInfo->ArgCount != ArgCount))
        {
            PrError (ASL_ERROR, ASL_MSG_EXISTING_NAME,
                THIS_TOKEN_OFFSET (Name));
        }

        goto ErrorExit;
    }

    DbgPrint (ASL_DEBUG_OUTPUT, PR_PREFIX_ID
        "Macro body: %s\n",
        AslGbl_CurrentLineNumber, BodyInSource);

    /* Add macro to the #define list */

    DefineInfo = PrAddDefine (Name, BodyInSource, FALSE);
    if (DefineInfo)
    {
        Body = UtLocalCalloc (strlen (BodyInSource) + 1);
        strcpy (Body, BodyInSource);

        DefineInfo->Body = Body;
        DefineInfo->Args = Args;
        DefineInfo->ArgCount = ArgCount;
    }

    return;


ErrorExit:
    ACPI_FREE (Args);
    return;
}


/*******************************************************************************
 *
 * FUNCTION:    PrDoMacroInvocation
 *
 * PARAMETERS:  TokenBuffer         - Current line buffer
 *              MacroStart          - Start of the macro invocation within
 *                                    the token buffer
 *              DefineInfo          - Info for this macro
 *              Next                - "Next" buffer from GetNextToken
 *
 * RETURN:      None
 *
 * DESCRIPTION: Expand a macro invocation
 *
 ******************************************************************************/

void
PrDoMacroInvocation (
    char                    *TokenBuffer,
    char                    *MacroStart,
    PR_DEFINE_INFO          *DefineInfo,
    char                    **Next)
{
    PR_MACRO_ARG            *Args;
    char                    *Token = NULL;
    UINT32                  TokenOffset;
    UINT32                  Length;
    UINT32                  i;
    UINT32                  Diff1;
    UINT32                  Diff2;

    /* Take a copy of the macro body for expansion */

    strcpy (AslGbl_MacroTokenBuffer, DefineInfo->Body);

    /* Replace each argument within the prototype body */

    Args = DefineInfo->Args;
    if (!Args->Name)
    {
        /* This macro has no arguments */

        Token = PrGetNextToken (NULL, PR_MACRO_ARGUMENTS, Next);

        if (!Token)
        {
            goto BadInvocation;
        }

        TokenOffset = (MacroStart - TokenBuffer);
        Length = Token - MacroStart + strlen (Token) + 1;

        PrReplaceData (
            &AslGbl_CurrentLineBuffer[TokenOffset], Length,
            AslGbl_MacroTokenBuffer, strlen (AslGbl_MacroTokenBuffer));
        return;
    }

    while (Args->Name)
    {
        /* Get the next argument from macro invocation */

        Token = PrGetNextToken (NULL, PR_MACRO_SEPARATORS, Next);
        if (!Token)
        {
            goto BadInvocation;
        }

        /*
         * Avoid optimizing using just 1 signed int due to specific
         * non-portable implementations of signed ints
         */
        Diff1 = strlen (Args->Name) > strlen (Token) ? strlen (Args->Name) -
            strlen (Token) : 0;

        Diff2 = strlen (Args->Name) < strlen (Token) ? strlen (Token) -
            strlen (Args->Name) : 0;

        /* Replace all instances of this argument */

        for (i = 0; i < Args->UseCount; i++)
        {
            /*
             * To test the output of the preprocessed macro function that
             * is passed to the compiler
             */

             /*
              * fprintf (stderr, "Current token = %s \t Current arg_name = %s \
              * \t strlen (Token) = %u \t strlen (Args->Name) = %u \t Offset = %u \
              * \t UseCount = %u \t", Token, Args->Name, strlen (Token), \
              *     strlen (Args->Name), Args->Offset[i], Args->UseCount);
              */

            AslGbl_MacroTokenReplaceBuffer = (char *) calloc ((strlen (AslGbl_MacroTokenBuffer)), sizeof (char));

            PrReplaceResizeSubstring (Args, Diff1, Diff2, i, Token);

            DbgPrint (ASL_DEBUG_OUTPUT, PR_PREFIX_ID
                "ExpandArg: %s\n",
                AslGbl_CurrentLineNumber, AslGbl_MacroTokenBuffer);
        }

        Args++;
    }

    if (!Token)
    {
        return;
    }

    /* Replace the entire macro invocation with the expanded macro */

    TokenOffset = (MacroStart - TokenBuffer);
    Length = Token - MacroStart + strlen (Token) + 1;

    PrReplaceData (
        &AslGbl_CurrentLineBuffer[TokenOffset], Length,
        AslGbl_MacroTokenBuffer, strlen (AslGbl_MacroTokenBuffer));

    return;

BadInvocation:
    PrError (ASL_ERROR, ASL_MSG_INVALID_INVOCATION,
        THIS_TOKEN_OFFSET (MacroStart));

    DbgPrint (ASL_DEBUG_OUTPUT, PR_PREFIX_ID
        "Bad macro invocation: %s\n",
        AslGbl_CurrentLineNumber, AslGbl_MacroTokenBuffer);
    return;
}
