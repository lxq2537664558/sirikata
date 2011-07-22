/*
  Copyright 2008 Chris Lambrou.
  All rights reserved.
*/

// This is the Tree grammar for Emerson language

tree grammar EmersonTree;

options
{
	//output=template;
	backtrack=true;
//	memoize=true;
	rewrite=true;
	ASTLabelType=pANTLR3_BASE_TREE;
//	ASTLabelType=pANTRL3_COMMON_TREE;
	tokenVocab=Emerson;
 language = C;
}

@header
{

    #include <stdlib.h>
    #include <string.h>
    #include <antlr3.h>
    #include "Util.h"
    #define APP(s) \
        { \
            const char* str = s; \
            int len = strlen(str); \
            int numNewlines = 0; \
            for (int i = 0; i < len; i++) \
                if (str[i] == '\n') \
                    numNewlines++; \
            program_string->append(program_string, str); \
            current_line += numNewlines; \
        }

    #define LINE(num) \
        { \
            if (lineIndex >= linesSize) { \
                linesSize *= 2; \
                emersonLines = (int*)realloc(emersonLines, linesSize); \
                jsLines = (int*)realloc(jsLines, linesSize); \
            } \
            emersonLines[lineIndex] = num; \
            jsLines[lineIndex] = current_line; \
            lineIndex++; \
        }

    #define CHECK_RESOURCES()                 \
    {                                         \
    APP("\nif ( ! __checkResources8_8_3_1__() )\n");  \
    APP("{ \nthrow new Error('__resource_error__'); \n}\n");   \
    }


    #define CHECK_KILL()\
    { \
    APP("\nif ( system.__isKilling() )\n");  \
    APP("{ \nthrow new Error('__killing__'); \n}\n");   \
    }


    
    #ifndef __SIRIKATA_INSIDE_WHEN_PRED__
    #define __SIRIKATA_INSIDE_WHEN_PRED__
    static bool insideWhenPred = false;
    #endif
}

@members    
{
    pANTLR3_STRING program_string;
    int current_line;
    int* emersonLines;
    int* jsLines;
    int linesSize;
    int lineIndex;
    extern pEmersonTree _treeParser;
    
}


program returns [pANTLR3_STRING return_str, int* emersonLines, int* jsLines, int numLines]
	:^(PROG 
            {
                pANTLR3_STRING_FACTORY factory = antlr3StringFactoryNew();
                program_string = factory->newRaw(factory);
                
                linesSize = 40;
                lineIndex = 0;
                emersonLines = (int*)malloc(linesSize * sizeof(int));
                jsLines = (int*)malloc(linesSize * sizeof(int));
    
                current_line = 1;
            }
            (
              sourceElements

            )?
         )
         {
            retval.return_str = program_string;
            retval.emersonLines = emersonLines;
            retval.jsLines = jsLines;
            retval.numLines = lineIndex;
         }
	;

        
sourceElements
    :(sourceElement{APP("\n"); })+  // omitting the LT 
    ;
	
sourceElement
    : functionDeclaration
    | statement{ APP(";"); }
    ;
	
// functions
functionDeclaration
	: ^( FUNC_DECL
              {
                LINE($FUNC_DECL.line);
                APP("function ");
              }
              Identifier
              {
                APP((const char*)$Identifier.text->chars);
                APP("( ");
              } 
              (formalParameterList)?
              {
                APP(" )");
                APP("\n{\n");
              }
              //for resource checks
              {
                 CHECK_RESOURCES();
              }
              functionBody
              {
                APP("\n}");
              }
            )
	;
	
functionExpression
	: ^( FUNC_EXPR 
	     {
               LINE($FUNC_EXPR.line);
               APP("function ");
             }
             (			
               Identifier
                 {
                   APP((const char*)$Identifier.text->chars);
                 }
             )? 
             {
               APP("( ");
             }
             (formalParameterList)?
             {
               APP("  )");
               APP("\n{\n");
             }
             //for resource checks
             {
                 CHECK_RESOURCES();
             }
             functionBody
             {
               APP("\n}");
             }
           )
       ;
	
formalParameterList
  : ^(FUNC_PARAMS
                (id1=Identifier {APP((const char*)$id1.text->chars); })
                
	       (
                 {
                   APP(", ");
                 }
                 id2=Identifier {APP((const char*)$id2.text->chars);}
		)*
      )
  ;
 


functionBody
	: sourceElements
	| EMPTY_FUNC_BODY 
	;

// statements
statement
    : noOpStatement
    | switchStatement
    | statementBlock
    | variableStatement
    | expressionStatement
    | ifStatement
    | iterationStatement
    | continueStatement
    | breakStatement
    | returnStatement
    | withStatement
    | throwStatement
    | tryStatement
    ;   

noOpStatement
        : ^(NOOP
          {
          }
        )
        ;
    
statementBlock
	: {APP(" {\n "); } statementList {  
            APP(" }\n");
        }
	;
	
statementList
	:  ^(
            SLIST 
            (statement
                {
			        APP("; \n");					  
                }
            )*
	    );
	
variableStatement
	:  ^( 
            VARLIST
            {
                LINE($VARLIST.line);
                APP("var ");
            }
            variableDeclarationList
        );
	
variableDeclarationList
	: variableDeclaration 
        (
            {
                APP(", ");
            }	
            variableDeclaration
        )*
	;
	
variableDeclarationListNoIn
	: variableDeclarationNoIn+
	;
	
variableDeclaration
	: ^(
            VAR
            Identifier 
            {
                APP((const char*)$Identifier.text->chars);
            }
            
            (
                {
                    APP(" = ");
                }
                initialiser
            )?
        )
	;
	
variableDeclarationNoIn
	: 
        ^(
            VAR
			{
                APP("var ");
			}
            Identifier 
            {
                APP((const char*)$Identifier.text->chars);
            }
            
            (
                {
                    APP(" = ");
                }
                initialiserNoIn
            )?
					
        )
	;
	
initialiser
	: expression 
	;
	
initialiserNoIn
	: expressionNoIn
	;
	
	
expressionStatement
	: expression
	;
	
ifStatement
	: ^(IF 
            {
                LINE($IF.line);
                APP(" if ");
                APP(" ( ");
            }
            expression 
            {
                APP(" ) \n{");
            }
            (statement 
            {
                APP(" \n");
            }
            )?
            {
                APP("\n}\n");
            }
            (
                {
                    APP("else \n{");
                }
                statement
                {
                   APP("\n}");
                }
            )?
        )
	;
	
iterationStatement
	: doWhileStatement
	| whileStatement
	| forStatement
	| forInStatement
	;
	
doWhileStatement
	: ^( 
            DO
            {
                LINE($DO.line); 
                APP(" do ");  						  
                //resource checking
                APP("{\n");
                CHECK_RESOURCES();
            }
            statement
            {
                APP("\n}\n");
                APP("while ( " );      
            }
            expression 
            {
                APP(" ) ");  
            }
        )
	;
	
whileStatement
	: ^(
            WHILE
            {
                LINE($WHILE.line); 
                APP(" while ( ");
            }
            expression 
            {
                APP(" ) "); 
                //resource checking
                APP("{\n");
                CHECK_RESOURCES();
            }
            statement
            {
                APP("\n}\n");
            }
        )
	;
	
forStatement
	: ^(
            FOR 
            {
                LINE($FOR.line); 
                APP(" for ( ");
            }
            (^(FORINIT forStatementInitialiserPart))?
            {
                APP(" ; ");
            }
            (^(FORCOND expression))? 
            {
                APP(" ; ");
            }
            (^(FORITER expression))?  
            {
                APP(" ) ");
                //resource checking
                APP("{\n");
                CHECK_RESOURCES();
            }
            statement
            {
                APP("\n}\n");
            }
        )
	;
	
forStatementInitialiserPart
    : expressionNoIn
    | ^(VARLIST variableDeclarationListNoIn)
    ;
	
forInStatement
	: ^(
        FORIN 
        {
            LINE($FORIN.line);
            APP(" for ( ");
        }

        forInStatementInitialiserPart 
        {
            APP(" in ");
        }
        expression 
        {
            APP(" ) ");
            //resource checking
            APP("{\n");
            CHECK_RESOURCES();
        }
        statement
        {
            APP("\n}\n");
        }
        )
	;
	
forInStatementInitialiserPart
	: leftHandSideExpression
	| ^(VAR variableDeclarationNoIn)
	;

continueStatement
    : ^(
        CONTINUE 
        {
            APP("continue ");
        } 
        (
            Identifier
            {
                APP((const char*)$Identifier.text->chars);
            }
        )?
      )
	;

breakStatement
    : ^(
        BREAK
        {
            LINE($BREAK.line);
            APP("break ");
        }
        (
            Identifier
            {
                APP((const char*)$Identifier.text->chars);
            }
        )?

        )
	;

returnStatement
    : ^(
        RETURN 
        {
            LINE($RETURN.line);
            APP("return ");
        }
        (		
            expression
        )?
       )
	;
	
withStatement
    : ^(WITH 
          {LINE($WITH.line); APP("with ( ");}

          expression 
          {APP(" )");}
          
          statement

    )
    ;

	
switchStatement
    : ^(
        SWITCH 
        {
            LINE($SWITCH.line); 
            APP(" switch ( ");
        }
        expression 
        {
            APP(" ) \n");
            APP("{ \n");
        }
        caseBlock
        {
            APP("} \n");
        }
       )
	;
	

caseBlock
    : ^(CASE_BLOCK
        caseClause?
    )
    | ^(CASE_BLOCK
        defaultClause
    )
    ;

    
// caseClause
//     : ^( CASE
//         {
//             APP("case ");
//         }
//         expression
//         {
//             APP(":");
//         }
//         statementList?
//         caseClause?
//         )
//     | ^( CASE
//         {
//             APP("case ");
//         }
//         expression
//         {
//             APP(":");
//         }
//         statementList?
//         defaultClause
//        )
//     ;



// caseClauseSeenDefault
//     : ^( CASE
//         {
//             APP("case ");
//         }
//         expression
//         {
//             APP(":");
//         }
//         statementList?
//         caseClauseSeenDefault?
//         )
//     ;



caseClause
    : ^( CASE
        {
            APP("case ");
        }
        ternaryExpression
        {
            APP(":");
        }
        statementList?
        caseClause?
        )
    | ^( CASE
        {
            APP("case ");
        }
        ternaryExpression
        {
            APP(":");
        }
        statementList?
        defaultClause
       )
    ;



caseClauseSeenDefault
    : ^( CASE
        {
            APP("case ");
        }
        ternaryExpression
        {
            APP(":");
        }
        statementList?
        caseClauseSeenDefault?
        )
    ;

    
defaultClause
    :^(DEFAULT
        {
            LINE($DEFAULT.line); 
            APP("default: ");
        }
        statementList?
        caseClauseSeenDefault?
      )
    ;


    
throwStatement
    : ^(THROW
        {
            LINE($THROW.line); 
            APP("throw ");
        }
        expression
       )
    ;


        

tryStatement
        : ^(TRY
            {
                LINE($TRY.line); 
                APP("try\n");
            }
            statementBlock
            catchFinallyBlock
           )
        ;

catchFinallyBlock
        : catchBlock finallyBlock?
        | finallyBlock
        ;

catchBlock
        : ^(CATCH
            {
                LINE($CATCH.line); 
                APP("catch (");
            }
            Identifier
            {
                APP((const char*)$Identifier.text->chars);
                APP( ")\n");
                APP(" {  \n");
                APP(" if ( system.__isResetting() ) \n { \n");
                APP("throw new Error('__resetting__');\n}\n");
                CHECK_RESOURCES();
                CHECK_KILL();
            }
            statementBlock
            {
                APP("  } \n");
            }
           )
        ;

finallyBlock
        : ^(FINALLY
            {
                LINE($FINALLY.line); 
                APP("finally \n");
                APP(" {  \n");  
                APP(" if ( system.__isResetting() ) \n { \n");
                APP("throw new Error('__resetting__');\n}\n");
                CHECK_RESOURCES();
                CHECK_KILL();
            }
            statementBlock
            {
                APP("}\n");
            }
           )
        ;


memAndCallExpression
: memberExpression
| callExpression 
;


catchClause
	: ^(CATCH 
	    {
                      LINE($CATCH.line); 
					  APP(" catch ( ");
					}
	    Identifier 
					{
					  APP((const char*)$Identifier.text->chars);
					  APP(" ) ");

					}
					
					statementBlock)
	   
	;
	
finallyClause
	: ^( FINALLY 
	   {
                  LINE($FINALLY.line); 
				  APP(" finally ");

				}
	   statementBlock 
				
				)
	;


// expressions
expression
        : ^(EXPR assignmentExpression)
	;
	
expressionNoIn
	: ^(EXPR_NO_IN  assignmentExpressionNoIn)
	;

assignmentExpression
scope
{
  const char* op;
}

        : conditionalExpression
        | ^(
            (
                ASSIGN                { $assignmentExpression::op = " = ";    }
                | MULT_ASSIGN         { $assignmentExpression::op = " *= ";  }
                | DIV_ASSIGN          { $assignmentExpression::op = " /= ";  }
                | MOD_ASSIGN          { $assignmentExpression::op = " \%= ";  }
                | ADD_ASSIGN          { $assignmentExpression::op = " += ";  } 
                | SUB_ASSIGN          { $assignmentExpression::op = " -= ";  } 
                | AND_ASSIGN          { $assignmentExpression::op = " &= "; }
                | EXP_ASSIGN          { $assignmentExpression::op  = " ^= "; }
                | OR_ASSIGN           { $assignmentExpression::op = " |= "; } 
            )

            leftHandSideExpression 
            {
                APP(" ");
                APP($assignmentExpression::op);
                APP(" ");
            }
            assignmentExpression
           )
          ;


          
assignmentExpressionNoIn
scope
{
  const char* op;
}
        : conditionalExpressionNoIn
        | ^(
          (
            ASSIGN        { $assignmentExpressionNoIn::op = " = ";    }
            | MULT_ASSIGN { $assignmentExpressionNoIn::op = " *= ";   }
            | DIV_ASSIGN  { $assignmentExpressionNoIn::op = " /= ";   }
            | MOD_ASSIGN  { $assignmentExpressionNoIn::op = " \%= ";  }
            | ADD_ASSIGN  { $assignmentExpressionNoIn::op = " += ";   } 
            | SUB_ASSIGN  { $assignmentExpressionNoIn::op = " -= ";   } 
            | AND_ASSIGN  { $assignmentExpressionNoIn::op = " &= ";   }
            | EXP_ASSIGN  { $assignmentExpressionNoIn::op  = " ^= ";  }
            | OR_ASSIGN   { $assignmentExpressionNoIn::op = " |= ";   } 
           )
           
           leftHandSideExpression
           {                                  
                 APP(" ");
                 APP($assignmentExpressionNoIn::op);
                 APP(" ");
           }

           assignmentExpressionNoIn
           )
       ;

       
leftHandSideExpression
	: callExpression
	| newExpression
	;
	
newExpression
	: memberExpression
	| ^( NEW newExpression)
	;
        
        
propertyReferenceSuffix1
: Identifier { APP((const char*)$Identifier.text->chars);} 
;

indexSuffix1
: expression
;

memberExpression
: primaryExpression
|functionExpression
| ^(DOT memberExpression { APP("."); } propertyReferenceSuffix1 )
| ^(ARRAY_INDEX memberExpression { APP("[ "); } indexSuffix1 { APP(" ] "); })
| ^(NEW { APP("new "); } memberExpression arguments)
| ^(DOT { APP(".");} memberExpression)
;

memberExpressionSuffix
	: indexSuffix
	| propertyReferenceSuffix 
	;

callExpression
 : ^(CALL memberExpression arguments) 
 | ^(ARRAY_INDEX callExpression {APP("[ "); } indexSuffix1 { APP(" ]"); })
 | ^(DOT callExpression { APP(".");} propertyReferenceSuffix1)
 | ^(CALL callExpression arguments)
;
	


callExpressionSuffix
	: arguments
	| indexSuffix
	| propertyReferenceSuffix
	;

arguments
  : ^(ARGLIST {APP("( )"); })
  | ^(ARGLIST 
       { APP("( "); }
       (expression)
       { APP(" )"); }
     )

  | ^(ARGLIST
      {
          APP("( ");
      }
      expression
      (
        {
            APP(", ");
        }
        expression
      )*
      {
        APP(" ) ");
      }
    )
    ;
 

indexSuffix
	: ^(ARRAY_INDEX expression)
	;	
	
propertyReferenceSuffix
	: ^(DOT Identifier)
	;
	
assignmentOperator
	: ASSIGN|MULT_ASSIGN|DIV_ASSIGN | MOD_ASSIGN| ADD_ASSIGN | SUB_ASSIGN|AND_ASSIGN|EXP_ASSIGN|OR_ASSIGN
	;

conditionalExpressionNoIn
        : msgRecvConstructNoIn
        ;

conditionalExpression
        : msgRecvConstruct
        ;
        
msgRecvConstruct
        : msgConstruct
        | ^(MESSAGE_RECV_AND_SENDER
            {
               APP("system.registerHandler( ");
            }
            msgRecvConstruct
            {
                APP(",");
            }
            msgConstruct
            {
                APP(",");
            }
            msgConstruct
            {
                APP(")");
            }
           )
        | ^(MESSAGE_RECV_NO_SENDER
            {
               APP("system.registerHandler( ");
            }
            msgRecvConstruct
            {
                APP(",");
            }
            msgConstruct
            {
                APP(", null)");
            }
           )
         ;

msgConstruct
        : msgSenderConstruct
        | ^(SEND_CONSTRUCT
            {
                APP("std.messaging.sendSyntax(");
            }
            msgConstruct
            {
                APP(",");
            }
            msgSenderConstruct
            {
                APP(")");
            }
           )
        ;
         

msgSenderConstruct
        :  ternaryExpression
        | ^(SENDER_CONSTRUCT
            {
                APP("std.messaging.SenderMessagePair(");
            }
            msgSenderConstruct
            {
                APP(",");
            }
            ternaryExpression
            {
                APP(")");
            }
           )
        ;

         
ternaryExpression
        : logicalORExpression
        | ^(TERNARYOP
            {
                APP( " ( ");
            }
            ternaryExpression
            {
                APP( " ) ? ( ");
            }
            //logicalORExpression
            assignmentExpression
            {
                APP(" ) : ( ");
            }
            //logicalORExpression
            assignmentExpression
            {
                APP(" ) " );
            }
           )
        ;

         
msgRecvConstructNoIn
        : msgConstructNoIn
        | ^(MESSAGE_RECV_AND_SENDER_NO_IN
            {
               APP("system.registerHandler( ");
            }
            msgRecvConstructNoIn
            {
                APP(",");
            }
            msgConstructNoIn
            {
                APP(",");
            }
            msgConstructNoIn
            {
                APP(")");
            }
           )
        | ^(MESSAGE_RECV_NO_SENDER_NO_IN
            {
               APP("system.registerHandler( ");
            }
            msgRecvConstructNoIn
            {
                APP(",");
            }
            msgConstructNoIn
            {
                APP(", null)");
            }
           )
         ;


msgConstructNoIn
        : msgSenderConstructNoIn
        | ^(SEND_CONSTRUCT_NO_IN
            {
                APP("std.messaging.sendSyntax(");
            }
            msgConstructNoIn
            {
                APP(",");
            }
            msgSenderConstructNoIn
            {
                APP(")");
            }
           )
        ;
         

msgSenderConstructNoIn
        :  ternaryExpressionNoIn
        | ^(SENDER_CONSTRUCT_NO_IN
            {
                APP("std.messaging.SenderMessagePair(");
            }
            msgSenderConstructNoIn
            {
                APP(",");
            }
            ternaryExpressionNoIn
            {
                APP(")");
            }
           )
        ;


ternaryExpressionNoIn
        : logicalORExpressionNoIn
        | ^(TERNARYOP_NO_IN
            {
                APP( " ( ");
            }
            ternaryExpressionNoIn
            {
                APP( " ) ? ( ");
            }
            assignmentExpressionNoIn
            {
                APP(" ) : ( ");
            }
            assignmentExpressionNoIn
            {
                APP(" ) " );
            }
           )
        ;




logicalANDExpression
	: bitwiseORExpression
	|^(AND logicalANDExpression { APP(" && ");} bitwiseORExpression)
	;


logicalORExpression
	: logicalANDExpression
	|^(OR logicalORExpression { APP(" || "); } logicalANDExpression)
	;
	
logicalORExpressionNoIn
	: logicalANDExpressionNoIn
	|^(OR logicalORExpressionNoIn{ APP(" || ");}logicalANDExpressionNoIn) 
	;
	

logicalANDExpressionNoIn
	: bitwiseORExpressionNoIn 
	|^(AND logicalANDExpressionNoIn {APP(" && ");} bitwiseORExpressionNoIn) 
	;
	
bitwiseORExpression
	: bitwiseXORExpression 
	|^(BIT_OR bitwiseORExpression { APP(" | "); } bitwiseXORExpression)
	;
	
bitwiseORExpressionNoIn
	: bitwiseXORExpressionNoIn 
	|^( BIT_OR bitwiseORExpressionNoIn {  APP(" | ");} bitwiseXORExpressionNoIn)
	;
	
bitwiseXORExpression
: bitwiseANDExpression 
| ^( EXP e=bitwiseXORExpression { APP(" ^ ");} bitwiseANDExpression)
;
	
bitwiseXORExpressionNoIn
	: bitwiseANDExpressionNoIn
	|^( EXP e=bitwiseXORExpressionNoIn { APP(" ^ ");}bitwiseANDExpressionNoIn) 
	;
	
bitwiseANDExpression
	: equalityExpression
	| ^(BIT_AND e=bitwiseANDExpression { APP(" & ");} equalityExpression) 
	;
	
bitwiseANDExpressionNoIn
	: equalityExpressionNoIn 
	| ^(BIT_AND e=bitwiseANDExpressionNoIn { APP(" & ");} equalityExpressionNoIn)
	;
	
equalityExpression
	: relationalExpression
        | ^(EQUALS
            {
                APP(" util.equal( ");
            }
            e=equalityExpression
            {
                APP(",");
            }
            relationalExpression
            {
                APP(")");
            }
           )
	| ^(NOT_EQUALS
            {
                APP(" util.notEqual( ");
            }
            e=equalityExpression
            {
                APP(" , ");
            }
            relationalExpression
            {
                APP(")");
            }
           )
	| ^(IDENT
            {
                APP(" util.identical( ");
            }
            e=equalityExpression
            {
                APP(" , ");
            }
            relationalExpression
            {
                APP(")");
            }
           )
	| ^(NOT_IDENT
            {
                APP(" util.notIdentical( ");
            }
            e=equalityExpression
            {
                APP(" , ");
            }
            relationalExpression
            {
                APP(")");
            }
           )
;

equalityExpressionNoIn
: relationalExpressionNoIn
  | ^(EQUALS
      {
          APP(" util.equal( ");        
      }
      equalityExpressionNoIn
      {
          APP(" , ");
      }
      relationalExpressionNoIn
      {
          APP(")");
      }
     )
  | ^(NOT_EQUALS
      {
          APP("util.notEqual(");
      }
      equalityExpressionNoIn
      {
          APP(" != ");
      }
      relationalExpressionNoIn
      {
          APP(")");      
      }
     )
  | ^(IDENT
      {
          APP("util.identical( ");
      }
      equalityExpressionNoIn
      {
          APP(" , ");
      }
      relationalExpressionNoIn
      {
          APP(")");
      }
     )
  | ^(NOT_IDENT
      {
         APP("util.notIdentical (");
      }
      equalityExpressionNoIn
      {
         APP(" , ");
      }
      relationalExpressionNoIn
      {
         APP(" )");
      }
     )
;
	

relationalOps
: LESS_THAN {   $relationalExpression::op = "<" ;}
| GREATER_THAN { $relationalExpression::op = ">" ;}
| LESS_THAN_EQUAL {$relationalExpression::op = "<=" ;} 
| GREATER_THAN_EQUAL { $relationalExpression::op = ">=" ;}
| INSTANCE_OF {$relationalExpression::op = "instanceOf" ;}
| IN {$relationalExpression::op = "in" ;} 
;

relationalExpression
scope
{
  const char* op;
}

	: additiveExpression 
	| 
	^(
	   relationalOps 
				e=relationalExpression
				{
				  APP(" ");
				  APP($relationalExpression::op );
				  APP(" ");
				}
				additiveExpression
			) 
	;

relationalOpsNoIn
: LESS_THAN {  $relationalExpressionNoIn::op = "<" ;}
| GREATER_THAN { $relationalExpressionNoIn::op = ">"; }
| LESS_THAN_EQUAL { $relationalExpressionNoIn::op = "<= " ;}
| GREATER_THAN_EQUAL { $relationalExpressionNoIn::op = ">=" ;}
| INSTANCE_OF    { $relationalExpressionNoIn::op = "instanceOf" ;}
;

relationalExpressionNoIn
scope
{
  const char* op;
}

	: additiveExpression 
	| 	^(
	     relationalOpsNoIn
						relationalExpressionNoIn
						{
						  APP(" ");
						  APP($relationalExpressionNoIn::op);
						  APP(" ");
						}
						additiveExpression
				)
	   
	;




additiveExpression
        : multiplicativeExpression
        | ^(ADD_OP 
             {
                APP("  util.plus( " );
             }
             e1=additiveExpression
             {
                APP(" , ");
             }
             multiplicativeExpression
             {
                APP( " ) ");
             }
            ) 
        | ^(SUB
            {
                APP("  util.sub( " );
            }
             e1=additiveExpression 
             {
                APP(" , ");
             }
             multiplicativeExpression
             {
                APP(" ) ");
             }
            ) 
	;


multiplicativeExpression
        : unaryExpression
        | ^( MULT
             {
                APP(" util.mul( ");
             } 
             multiplicativeExpression 
             {
                APP(" , ");
             }
             unaryExpression
             {
                APP(" ) ");
             }
            )
        | ^( DIV
            {
                APP(" util.div( ");
            }
            multiplicativeExpression
            {
                APP(" , ");
            }
            unaryExpression
            {
                APP(" ) ");
            }
           )
        | ^( MOD
            {       
                APP(" util.mod( ");
            }
            multiplicativeExpression
            {
                APP(" , ");
            }
            unaryExpression
            {
                APP(" ) ");
            }
           )
        ;

unaryOps
: DELETE_OP 
| VOID
| TYPEOF
| PLUSPLUS
| MINUSMINUS
| UNARY_PLUS
| UNARY_MINUS
| COMPLEMENT
| NOT
;


unaryExpression
        : postfixExpression 
	| ^(
	
	    (
				   DELETE_OP          {  APP("delete ");}
       | VOID          {   APP("void");}
       | TYPEOF        {  APP("typeof ");}
       | PLUSPLUS      {  APP("++");}
       | MINUSMINUS    {  APP("--");}
       | UNARY_PLUS    {  APP("+");}
       | UNARY_MINUS   {  APP("-");}
       | COMPLEMENT    {  APP("~");}
       | NOT           {  APP("!");}
	
					)

	    unaryExpression
				)
	;
	

postfixExpression
        :leftHandSideExpression
        | ^(MINUSMINUS leftHandSideExpression) { APP("--");}
	| ^(PLUSPLUS leftHandSideExpression) {APP("++");}
        ;

primaryExpression
	: 'this' {APP("this");}
	| Identifier 
	  { 
            APP((const char*)$Identifier.text->chars);
	  }
        | dollarExpression
	| literal
	| arrayLiteral
	| objectLiteral
        | patternLiteral
	| ^(PAREN { APP("( "); } expression { APP(" )");})
        | vectorLiteral
	;



vectorLiteral
        : ^(VECTOR
            {
                APP("( new util.Vec3(");
            }
            (exp1=vectorLiteralField
              {
                  APP(",");
              }
            )
            (exp2=vectorLiteralField
              {
                  APP(",");
              }
            )
            (exp3=vectorLiteralField
              {
                  APP(") )");
              }
            )
           )
        ;       


        
vectorLiteralField
        : additiveExpression
//        : ternaryExpression
        | NumericLiteral {APP((const char*)$NumericLiteral.text->chars);}
        | callExpression
        | memberExpression
        ;

        
dollarExpression
        : ^(DOLLAR_EXPRESSION
            {
                if (insideWhenPred)
                    APP("'),");

            }
            Identifier
            {
                APP((const char*)$Identifier.text->chars);

                if (insideWhenPred)
                   APP(",util.create_quoted('");
            }
         )
         ;
        

        
// arrayLiteral definition.
arrayLiteral
  : ^(ARRAY_LITERAL {APP("[ ]"); })
  | ^(ARRAY_LITERAL 
      { APP("[ "); }
       (expression)
      { APP(" ]"); }
     )
  | ^(ARRAY_LITERAL
     {
        APP("[ ");
     }
     expression
    	
     (
      {
         APP(", ");
      }
      expression
     )*
     {
         APP(" ] ");
     }
    )
    ;

// objectLiteral definition.
objectLiteral
  :^(OBJ_LITERAL {APP("{ }");} )
  |^(OBJ_LITERAL 
	    { APP("{ "); } 
            
            (propertyNameAndValue)

           {  APP(" }"); }
    )
	|^(OBJ_LITERAL 
	   
				{ APP("{ ");}
				propertyNameAndValue
				( 
				  { 
					  APP(", "); 
					} 
				
				  propertyNameAndValue
				)*

      	{ 
				  APP(" } "); 
				
				}

			)
				
	;
	
// patternLiteral definition.
patternLiteral

  :^(PATTERN_LITERAL {APP("new util.Pattern()");} )
  |^(PATTERN_LITERAL 
      nameValueProto
    )
  |^(PATTERN_LITERAL 
	   
				{ APP("[ ");}
				  nameValueProto
				( 
				  { 
					  APP(", "); 
					} 
				
				  nameValueProto
				)*

      	                      { 
				  APP(" ] "); 
				
				}

			)
				
	;
	

nameValueProto
  : ^(NAME_VALUE_PROTO
          {
            APP("new util.Pattern( ");
          }
        ^(NAME
          propertyName
         )
        
         (
         ^(VALUE
            {
              APP(", ");
            }

            expression
         ))?

         (
           
         ^(PROTO
            {
              APP(", ");
            }
            expression
         ) )?

         {
            APP(" )");
         }
      )
  | ^(BLANK_NAME_VAL_PROT
        {
            APP("new util.Pattern()");
        }
     )
  ;

propertyNameAndValue
	: ^(NAME_VALUE 
          propertyName 
           {	APP(" : ");}
	  expression)
	;

propertyName
	: Identifier {  APP((const char*)$Identifier.text->chars); }
	| StringLiteral
          {
             APP((const char*)$StringLiteral.text->chars);  
          }
	| NumericLiteral {APP((const char*)$NumericLiteral.text->chars);}
	;

// primitive literal definition.
literal
	: 'null' {   APP("null");}
	| 'true' {   APP("true"); }
	| 'false'{  APP("false");}
	| StringLiteral
          {
              const char* input = (const char*)$StringLiteral.text->chars;
              int len = $StringLiteral.text->len;
              char firstChar = *input;
              if(firstChar == '@')
              {
                std::string str_input(input,len);
                str_input = str_input.substr(1, str_input.size() -2);
                std::string escaped = emerson_escapeMultiline(str_input.c_str());
                APP("\"");
                APP(escaped.c_str());
                APP("\"");
              }
              else APP((const char*)$StringLiteral.text->chars);
        }
	| NumericLiteral {APP((const char*)$NumericLiteral.text->chars);}
	;
	



