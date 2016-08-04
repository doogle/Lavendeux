from lavendeux import Types, Errors

def call(args):
        # Check number of arguments
        if len(args) != 2:
            return (Types.ERROR, Errors.INVALID_ARGS)

        # Return answer
    return (Types.FLOAT, args[0] + args[1])

def decorate(value):
        return "Decorated!"

def help():
    return "Help message"