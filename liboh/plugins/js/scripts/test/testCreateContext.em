///This function tests the createContext code.  From system, create a context, print in the context, and execute from the new context


system.print("\n\nRunning test/testCreatecontext.em.  Tests creating a context and printing from the context \n\n");


system.print("\nCreate context\n");

var newContext = system.create_context();

newContext.print("\n\nPRINTING FROM NEW_CONTEXT\n\n");