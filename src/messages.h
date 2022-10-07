/* messages.h, symbolic constants for edbrowse messages. */

/*********************************************************************
Symbolic constants for message numbers.
These become the indexes for the messages in lang/msg-en,
or in lang/msg-xx for language xx.
The externs below correspond to the languages supported.
The array of messages is selected by $LANG.
See selectLanguage() in messages.c.
EdbrowseMessageCount is the number of edbrowse messages, including some
blank spaces for messages no longer used.
Each file lang/msg-* must have exactly this many lines.
*********************************************************************/

#define EdbrowseMessageCount 800

// English
extern const char *msg_en[], ebrc_en[], qrg_en[];

// French, Erwin Bliesenick, erwinb@no-log.org
extern const char *msg_fr[], ebrc_fr[], qrg_fr[];

// Brazilian Portuguese, Cleverson Casarin, clcaul@gmail.com
extern const char *msg_pt_br[], ebrc_pt_br[], qrg_pt_br[];

// German, Sebastian Humenda, shumenda@gmx.de
extern const char *msg_de[], ebrc_de[], qrg_de[];

// Polish, Wojciech Gac, wojciech.s.gac@gmail.com
extern const char *msg_pl[], ebrc_pl[], qrg_pl[];

// Russian, Wojciech Gac, wojciech.s.gac@gmail.com
extern const char *msg_ru[], ebrc_ru[], qrg_ru[];

// Italian, Enrico Mioso, mrkiko.rs@gmail.com
extern const char *msg_it[], ebrc_it[], qrg_it[];

/*********************************************************************
This file and lang/msg-en line up.
The constant at line 101 of messages.h corresponds
to the English string at line 1 of lang/msg-en.
to keep this alignment, even if you run this file through indent,
this comment block is just the right size.
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*
*********************************************************************/

enum {
	MSG_EndFile,
	MSG_NoFile,
	MSG_Redirect,
	MSG_SubLocal,
	MSG_SubGlobal,
	MSG_CaseSen,
	MSG_CaseIns,
	MSG_DirReadonly,
	MSG_DirWritable,
	MSG_DirX,
	MSG_RedirectionOff,
	MSG_RedirectionOn,
	MSG_RefererOff,
	MSG_RefererOn,
	MSG_JavaOff,
	MSG_JavaOn,
	MSG_BinaryIgnore,
	MSG_BinaryDetect,
	MSG_PassiveMode,
	MSG_ActiveMode,
	MSG_JSCloseSessions,
	MSG_CertifyOff,
	MSG_CertifyOn,
	MSG_HiddenOff,
	MSG_HiddenOn,
	MSG_DebugCloneOff,
	MSG_DebugCloneOn,
	MSG_MarkersOff,
	MSG_MarkersOn,
	MSG_NoFromLine,
	MSG_NJNoAction,
	MSG_NJGoing,
	MSG_RedirectionInterrupted,
	MSG_Empty,
	MSG_SessionNew,
	MSG_NoTrailing,
	MSG_DirMode,
	MSG_BinaryData,
	MSG_OK,
	MSG_BinaryBrackets,
	MSG_NoTitle,
	MSG_NoDesc,
	MSG_NoKeywords,
	MSG_SessionX,
	MSG_MailHowto,
	MSG_LineUpdate1,
	MSG_ImapMessageHelp,
	MSG_ImapSearchHelp,
	MSG_NoMessages,
	MSG_EnterInterrupt,
	MSG_Ready,
	MSG_EnterNull,
	MSG_WebAuthorize,
	MSG_RedirectMany,
	MSG_Success,
	MSG_Directory,
	MSG_RedirectDelayed,
	MSG_UserName,
	MSG_UserNameLong,
	MSG_Password,
	MSG_PasswordLong,
	MSG_String,
	MSG_NoDirNoList,
	MSG_ErrorMessageLong,
	MSG_LineX,
	MSG_NoDir,
	MSG_BufferUpdated,
	MSG_NoOptions,
	MSG_FormReset,
	MSG_FormSubmit,
	MSG_EBRC_Outtype,
	MSG_Many,
	MSG_Recommended,
	MSG_Close,
	MSG_NoOptionsMatch,
	MSG_MailSending,
	MSG_MailSent,
	MSG_Junk,
	MSG_Quit,
	MSG_Next,
	MSG_Delete,
	MSG_PluginsOff,
	MSG_PluginsOn,
	MSG_JSEngineRun,
	MSG_EndMessage,
	MSG_NYI,
	MSG_FileName,
	MSG_FileExists,
	MSG_Abbreviated,
	MSG_AttEmpty,
	MSG_Att,
	MSG_AttNoBuffer,
	MSG_AttNoCopy,
	MSG_AttNoSave,
	MSG_AttNoWrite,
	MSG_NoMail,
	MSG_MessagesX,
	MSG_NoConfig,
	MSG_LSBadChar,
	MSG_MailHelp,
	MSG_NoCreate,
	MSG_NoWrite,
	MSG_MailSaved,
	MSG_Appended,
	MSG_PageDone,
	MSG_TempDir,
	MSG_NoErrors,
	MSG_ProxyAuth,
	MSG_SessionInactive,
	MSG_Session0,
	MSG_SessionHigh,
	MSG_SessionCurrent,
	MSG_ExpectW,
	MSG_ExpectWX,
	MSG_LineLimit,
	MSG_PathNameLong,
	MSG_DirNoWrite,
	MSG_NoRecycle,
	MSG_NoRemove,
	MSG_NoMoveToTrash,
	MSG_DestInBlock,
	MSG_NoChange,
	MSG_Join1,
	MSG_DBOtherFile,
	MSG_DBOtherTable,
	MSG_MissingFileName,
	MSG_DomainEmpty,
	MSG_NoWriteURL,
	MSG_WriteDB,
	MSG_IrcCommand,
	MSG_NoCreate2,
	MSG_NoWrite2,
	MSG_SessionBackground,
	MSG_OutOfRange,
	MSG_ShellNull,
	MSG_NoLabel,
	MSG_BadDelimit,
	MSG_NoSearchString,
	MSG_NoReplaceString,
	MSG_RexpLong,
	MSG_LineBackslash,
	MSG_UnexpectedRight,
	MSG_RexpDollar,
	MSG_RexpModifier,
	MSG_NoBracket,
	MSG_NoParen,
	MSG_EmptyBuffer,
	MSG_RexpError,
	MSG_RexpError2,
	MSG_NotFound,
	MSG_LineHigh,
	MSG_LineLow,
	MSG_RexpMissing,
	MSG_NoDelimit,
	MSG_NoMatchG,
	MSG_AllMatchV,
	MSG_NotModifiedG,
	MSG_Interrupted,
	MSG_NoMatch,
	MSG_GopherEmptyDir,
	MSG_NoAgent,
	MSG_CDGetError,
	MSG_CDSetError,
	MSG_CDInvalid,
	MSG_AudioEmpty,
	MSG_AudioBrowse,
	MSG_AudioDir,
	MSG_AudioDB,
	MSG_NoSuffix,
	MSG_SuffixLong,
	MSG_SuffixBad,
	MSG_NoRefresh,
	MSG_NoDB,
	MSG_NoBrowse,
	MSG_NoSlash,
	MSG_YesSlash,
	MSG_SMBadChar,
	MSG_BalanceChar,
	MSG_NoAppendURL,
	MSG_BalanceAmbig,
	MSG_BalanceNothing,
	MSG_Unbalanced,
	MSG_Personalize,
	MSG_SessionDir,
	MSG_NoFileName,
	MSG_EndBuffer,
	MSG_EndJoin,
	MSG_BadRange,
	MSG_BreakDir,
	MSG_BreakDB,
	MSG_BreakBrowse,
	MSG_UnknownCommand,
	MSG_DirCommand,
	MSG_DBCommand,
	MSG_BrowseCommand,
	MSG_AtLine0,
	MSG_NoSpaceAfter,
	MSG_GlobalCommand,
	MSG_BadDest,
	MSG_TextAfter,
	MSG_NoUndo,
	MSG_EnterKAZ,
	MSG_RangeLabel,
	MSG_Backup0,
	MSG_NoBufferExtraWindow,
	MSG_QAfter,
	MSG_DirRename,
	MSG_TableRename,
	MSG_BufferAppend,
	MSG_NoFileSpecified,
	MSG_NoDirWrite,
	MSG_NoDBWrite,
	MSG_ArrowAfter,
	MSG_NoPrevious,
	MSG_MAfter,
	MSG_MovedSession,
	MSG_NoBackup,
	MSG_RangeCmd,
	MSG_DBG,
	MSG_RangeI,
	MSG_IG,
	MSG_BufferXEmpty,
	MSG_BufferXLines,
	MSG_NoOpen,
	MSG_NoRead,
	MSG_WebProtBad,
	MSG_InputCR,
	MSG_GopherDownload,
	MSG_AlreadyInBuffer,
	MSG_BrowseBinary,
	MSG_BrowseEmpty,
	MSG_Unbrowsable,
	MSG_BrowseAlready,
	MSG_NoLabel2,
	MSG_BrowseI,
	MSG_InsertFunction,
	MSG_LineUpdateRange,
	MSG_CNYI,
	MSG_XOutOfRange,
	MSG_OptMatchNone,
	MSG_OptMatchMany,
	MSG_IsButton,
	MSG_SubmitButton,
	MSG_ResetButton,
	MSG_BigFile,
	MSG_Readonly,
	MSG_InputNewline,
	MSG_InputLong,
	MSG_InputRadio,
	MSG_ClearRadio,
	MSG_FileAccess,
	MSG_NumberExpected,
	MSG_SessionNull,
	MSG_FilePost,
	MSG_NoButton,
	MSG_NotInForm,
	MSG_ButtonNoJS,
	MSG_FormNoURL,
	MSG_FormBadURL,
	MSG_NJNoForm,
	MSG_BecameInsecure,
	MSG_SubmitProtBad,
	MSG_InputRange,
	MSG_InputRange2,
	MSG_ManyEmptyStrings,
	MSG_DownAbort,
	MSG_NoDir0,
	MSG_SubNumbersMany,
	MSG_SubSuffixBad,
	MSG_SubNumberG,
	MSG_BreakLong,
	MSG_ReplaceNewline,
	MSG_ReplaceNull,
	MSG_DirNameBad,
	MSG_DestFileExists,
	MSG_NoRename,
	MSG_InputNull2,
	MSG_InputNewline2,
	MSG_SSLConnectError,
	MSG_NoUp,
	MSG_NoDown,
	MSG_notused282,
	MSG_notused283,
	MSG_notused284,
	MSG_WebRead,
	MSG_BadURL,
	MSG_DownSuccess,
	MSG_IdentifyHost,
	MSG_NoPlain,
	MSG_BufferPreload,
	MSG_DownProgress,
	MSG_WebConnect,
	MSG_Down,
	MSG_NoCertify,
	MSG_Inaccess,
	MSG_ImapReadHelp,
	MSG_Authorize2,
	MSG_LoginAbort,
	MSG_SearchQuote,
	MSG_EndFolder,
	MSG_DownForeground,
	MSG_DownBackground,
	MSG_Complete,
	MSG_Failed,
	MSG_FTPConnect,
	MSG_FTPTransfer,
	MSG_NoFolders,
	MSG_SelectFolder,
	MSG_InProgress,
	MSG_Stop,
	MSG_FetchN,
	MSG_NoAttachments,
	MSG_EBRC_KeyInFunc,
	MSG_NoTable,
	MSG_FTPSession,
	MSG_Push,
	MSG_DBNotCompiled,
	MSG_ABNoAlias,
	MSG_ABNoColon,
	MSG_ABAliasLong,
	MSG_ABMailLong,
	MSG_ABNoAt,
	MSG_ABMailSpaces,
	MSG_AbMailUnprintable,
	MSG_ABUnterminated,
	MSG_NoFolderMatch,
	MSG_ManyFolderMatch,
	MSG_Search,
	MSG_Envelope,
	MSG_MoveTo,
	MSG_SectionIgnored,
	MSG_FileXEmpty,
	MSG_SubjectStart,
	MSG_ProgressQuiet,
	MSG_SubjectLong,
	MSG_ProgressDots,
	MSG_SigRegular,
	MSG_SigAccess,
	MSG_MailIrc,
	MSG_AttAlternate,
	MSG_RecipMany,
	MSG_ABMissing,
	MSG_ABNoAlias2,
	MSG_RecipNone,
	MSG_AttSessionEmpty,
	MSG_ReIrc,
	MSG_AttRegular,
	MSG_AttEmpty2,
	MSG_LineDelete1,
	MSG_LineDelete2,
	MSG_LineAdd1,
	MSG_LineAdd2,
	MSG_ProgressCount,
	MSG_MailAccountsNone,
	MSG_MailAccountBad,
	MSG_MailBrowse,
	MSG_MailDB,
	MSG_MailDir,
	MSG_MailBinary2,
	MSG_MailEmpty,
	MSG_RecipNone2,
	MSG_MailFirstCC,
	MSG_AttLineX,
	MSG_MailAccountBadLineX,
	MSG_TempNone,
	MSG_MailFirstLine,
	MSG_ConvUtf16,
	MSG_RecipNone3,
	MSG_ProtExpected,
	MSG_BadProt,
	MSG_BadPort,
	MSG_DomainLong,
	MSG_UserNameLong2,
	MSG_PasswordLong2,
	MSG_LineUpdate2,
	MSG_LineUpdate3,
	MSG_NoFunction,
	MSG_BadFunctionName,
	MSG_NoSuchFunction,
	MSG_ManyArgs,
	MSG_NoArgument,
	MSG_NoSpawn,
	MSG_TempNoCreate2,
	MSG_ManyTables,
	MSG_RegularFile,
	MSG_ConvUtf32,
	MSG_NoRead2,
	MSG_NoAccess,
	MSG_NoEnvVar,
	MSG_NoTilde,
	MSG_Submit,
	MSG_FrameNotHTML,
	MSG_Secure,
	MSG_Bymail,
	MSG_Reset,
	MSG_ShellExpand,
	MSG_FrameNoURL,
	MSG_ShellManyMatch,
	MSG_ShellLineLong,
	MSG_DBUnspecified,
	MSG_DBConnect,
	MSG_DBUnexpected,
	MSG_DBNoKey,
	MSG_DBColumnLong,
	MSG_DBSyntax,
	MSG_DBColRange,
	MSG_DBManyColumns,
	MSG_DBNoColumn,
	MSG_DBNoTable,
	MSG_DBBadColumn,
	MSG_DBManyBlobs,
	MSG_DBBackslash,
	MSG_DBNewline,
	MSG_DBAddField,
	MSG_DBLostField,
	MSG_DBNoKeyCol,
	MSG_DBMisc,
	MSG_DBMassDelete,
	MSG_DBChangeKey,
	MSG_DBChangeBlob,
	MSG_DBChangeText,
	MSG_DBDeleteCount,
	MSG_DBInsertCount,
	MSG_DBUpdateCount,
	MSG_DBRefInt,
	MSG_DBLocked,
	MSG_DBPerms,
	MSG_DBDeadlock,
	MSG_DBNotNull,
	MSG_DBCheck,
	MSG_DBTimeout,
	MSG_DBView,
	MSG_Disabled,
	MSG_CurlNoAuthNegotiate,
	MSG_CurlAuthNegotiate,
	MSG_GetLocalCSS,
	MSG_GetCSS,
	MSG_GetCSS2,
	MSG_DeminOff,
	MSG_DeminOn,
	MSG_DebugThrowOff,
	MSG_DebugThrowOn,
	MSG_DebugCSSOff,
	MSG_DebugCSSOn,
	MSG_LineAddZ1,
	MSG_LineAddZ2,
	MSG_LineAddZ3,
	MSG_LineUpdateZ1,
	MSG_LineUpdateZ2,
	MSG_LineUpdateZ3,
	MSG_LineDeleteZ1,
	MSG_LineDeleteZ2,
	MSG_LineDeleteZ3,
	MSG_PluginFile,
	MSG_NotPlayer,
	MSG_NotConverter,
	MSG_HoverOff,
	MSG_HoverOn,
	MSG_Batch,
	MSG_From,
	MSG_AttAfterChars,
	MSG_AttBad64,
	MSG_GlobalCommand2,
	MSG_Doubleclick,
	MSG_OptionSync,
	MSG_MoveFileSystem,
	MSG_MoveError,
	MSG_GetLocalJS,
	MSG_GetJS,
	MSG_GetJS2,
	MSG_NJNoOnclick,
	MSG_NJNoOnchange,
	MSG_NJNoReset,
	MSG_NJNoSubmit,
	MSG_LostTag,
	MSG_ExpGo,
	MSG_GarbledRefresh,
	MSG_NewWindow,
	MSG_RedirectNoURL,
	MSG_HTTPError,
	MSG_NoFrame1,
	MSG_NoFrame2,
	MSG_HelpOn,
	MSG_FTPDownload,
	MSG_Abort,
	MSG_SCPDownload,
	MSG_EBRC_Nulls,
	MSG_EBRC_NoFnName,
	MSG_EBRC_FnDigit,
	MSG_EBRC_FnTooLong,
	MSG_EBRC_SyntaxErr,
	MSG_EBRC_NoCondFile,
	MSG_EBRC_NoMatchStr,
	MSG_EBRC_MatchNowh,
	MSG_EBRC_NoAgent,
	MSG_EBRC_BadKeyword,
	MSG_EBRC_MailAttrOut,
	MSG_EBRC_MimeAttrOut,
	MSG_EBRC_TableAttrOut,
	MSG_EBRC_MailAttrIn,
	MSG_EBRC_MimeAttrIn,
	MSG_EBRC_TableAttrIn,
	MSG_EBRC_NoAttr,
	MSG_EBRC_ManyCols,
	MSG_EBRC_KeyNotNb,
	MSG_EBRC_KeyOutRange,
	MSG_EBRC_AbNotFile,
	MSG_CompileError,
	MSG_EBRC_NotDir,
	MSG_EBRC_ManyAgents,
	MSG_EBRC_JarNotFile,
	MSG_EBRC_JarNoWrite,
	MSG_EmailInput,
	MSG_EBRC_DomainDot,
	MSG_NoAccessSecure,
	MSG_EBRC_SSLNoFile,
	MSG_EBRC_SSLNoRead,
	MSG_EBRC_KeywordNYI,
	MSG_EBRC_SevDefaults,
	MSG_EBRC_NoInserver,
	MSG_EBRC_NoOutserver,
	MSG_EBRC_NoLogin,
	MSG_EBRC_NPasswd,
	MSG_EBRC_NoFrom,
	MSG_EBRC_NoReply,
	MSG_EBRC_NoType,
	MSG_EBRC_NDesc,
	MSG_EBRC_NoSuffix,
	MSG_EBRC_NoProgram,
	MSG_EBRC_NoTblName,
	MSG_EBRC_NoShortName,
	MSG_EBRC_NColumns,
	MSG_EBRC_UnexpBrace,
	MSG_EBRC_UnexElse,
	MSG_EBRC_GarblText,
	MSG_EBRC_FnNotStart,
	MSG_EBRC_StatNotInFn,
	MSG_EBRC_ManyAcc,
	MSG_EBRC_ManyTypes,
	MSG_EBRC_ManyTables,
	MSG_UrlInput,
	MSG_EBRC_TooDeeply,
	MSG_EBRC_FnNotClosed,
	MSG_EBRC_MNotClosed,
	MSG_NotHome,
	MSG_NotDir,
	MSG_NoMailAcc,
	MSG_BadAccNb,
	MSG_Usage,
	MSG_MinOneRec,
	MSG_MinOneRecBefAtt,
	MSG_ManyOpen,
	MSG_InvalidSession,
	MSG_InvalidLineNb,
	MSG_MetaChar,
	MSG_SessionOutRange,
	MSG_DoubleInit,
	MSG_QuitNoActive,
	MSG_EmptyPiece,
	MSG_NoNlOnDir,
	MSG_NoClosingLine,
	MSG_NoTagFound,
	MSG_NoRebCookie,
	MSG_Reverse,
	MSG_FetchNotBackgnd,
	MSG_NoMailDir,
	MSG_NoDirChange,
	MSG_JavaBlock,
	MSG_LogPass,
	MSG_SortAlpha,
	MSG_SortSize,
	MSG_SortDate,
	MSG_ConvUnix,
	MSG_ConvDos,
	MSG_BadTagCode,
	MSG_IntForce,
	MSG_HtmlNotreentrant,
	MSG_UnexSubmitForm,
	MSG_NullListInform,
	MSG_DebugEventOff,
	MSG_DebugEventOn,
	MSG_JavaContextError,
	MSG_JSDownForeground,
	MSG_JSDownBackground,
	MSG_CopyFail,
	MSG_TimersOff,
	MSG_TimersOn,
	MSG_MemAllocError,
	MSG_MemCallocError,
	MSG_ReallocP,
	MSG_Realloc0,
	MSG_ErrorRealloc,
	MSG_NullStrList,
	MSG_NullStrListCI,
	MSG_NullCharInList,
	MSG_NullPtr,
	MSG_IoctlError,
	MSG_OpenFail,
	MSG_CreateFail,
	MSG_InvalidFopen,
	MSG_BadDirSlash,
	MSG_BadSlash,
	MSG_DecodePost,
	MSG_WrapOff,
	MSG_WrapOn,
	MSG_NoColumnHeaders,
	MSG_ReDir,
	MSG_ReDB,
	MSG_ReEmpty,
	MSG_ReBinary,
	MSG_ReSubjectReply,
	MSG_ReNoID,
	MSG_ReNoInfo,
	MSG_ListControl,
	MSG_ListNA,
	MSG_PcreUtf8,
	MSG_Conv8859,
	MSG_ConvUtf8,
	MSG_IUConvertOff,
	MSG_IUConvertOn,
	MSG_CopyMoveDir,
	MSG_NoDirDelete,
	MSG_LibcurlNoInit,
	MSG_FTPEmptyDir,
	MSG_ReAscii,
	MSG_ReUtf8,
	MSG_BadUtf8String,
	MSG_DBManyKeyCol,
	MSG_DBNextSerial,
	MSG_ShowTables,
	MSG_DBNoSource,
	MSG_DBConnecting,
	MSG_Row,
	MSG_Rows,
	MSG_Selected,
	MSG_Inserted,
	MSG_Updated,
	MSG_Deleted,
	MSG_ProcExec,
	MSG_Table,
	MSG_UnterminatedSelect,
	MSG_DBNoWhere,
	MSG_RemoteAccessDenied,
	MSG_AuthRequired,
	MSG_NoMem,
	MSG_CurlVersion,
	MSG_Timeout,
	MSG_CurlSendData,
	MSG_CurlCatchAll,
	MSG_Fkeys,
	MSG_FetchBlobOff,
	MSG_FetchBlobOn,
	MSG_SSLCurlError,
	MSG_InputTTY,
	MSG_InputReadLine,
	MSG_DebugTraceOff,
	MSG_DebugTraceOn,
	MSG_SystemCmdFail,
	MSG_ColorOff,
	MSG_ColorOn,
	MSG_ShowLast,
	MSG_ShowFirst,
	MSG_Previous,
	MSG_NoPrevMail,
	MSG_Restart,
	MSG_EBRC_MailDirect,
	MSG_OptionC,
	MSG_FileC,
	MSG_ReqField,
	MSG_Limit,
	MSG_TextRec,
	MSG_AtSyntax,
	MSG_EBRC_EmojiNotFile,
	MSG_EmojiBadControl,
	MSG_EmojiBadChar,
	MSG_EmojiOverGroup,
	MSG_EmojiUnicode,
	MSG_EmojiSyntax,
	MSG_OptStartNone,
	MSG_OptStartMany,
	MSG_EndBackslash,
	MSG_UpdateColname,
	MSG_FirstColumns,
	MSG_BreakRow,
	MSG_DelUnfold,
	MSG_EbreOff,
	MSG_EbreOn,
	MSG_EPlus,
	MSG_EMinus,
	MSG_EBRC_KeyNoFile,
	MSG_EBRC_KeyNoRead,
	MSG_DirNamesOnlyOff,
	MSG_DirNamesOnlyOn,
	MSG_DebugTagsOff,
	MSG_DebugTagsOn,
	MSG_FormatFlowedOff,
	MSG_FormatFlowedOn,
	MSG_AudioIrc,
	MSG_BreakIrc,
	MSG_IrcRename,
	MSG_IrcEntire,
	MSG_IrcUsage,
	MSG_IrcCompat,
	MSG_DebugLayOff,
	MSG_DebugLayOn,
	MSG_NoInputFields,
	MSG_NoLinks,
	MSG_NoButtons,
	MSG_NoImages,
	MSG_hold710,
	MSG_ManyInputFields,
	MSG_ManyLinks,
	MSG_ManyButtons,
	MSG_ManyImages,
	MSG_hold715,
};
