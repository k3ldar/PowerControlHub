using CommandLinePlus;
using SmartFuseSync;

IConsoleProcessorFactory factory = new ConsoleProcessorFactory();
object[] processors = [new SerialConfigProcessor(), new WifiConfigProcessor()];

IConsoleProcessor consoleProcessor = factory.Create("SmartFuseSync", processors);

switch (consoleProcessor.Run(out int resultCode))
{
    case RunResult.CandidateFound:
        Console.WriteLine("finished");
        break;

    case RunResult.DisplayHelp:
        break;

    default:
        Console.WriteLine("Invalid usage, please use -? for help");
        break;
}

return resultCode;