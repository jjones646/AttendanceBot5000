using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Net;
using System.IO;
using System.IO.Ports;

namespace AttendanceBot5000
{
    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("Booting Attendance Bot 5000");
            Console.WriteLine("===========================\n");
            // Get a list of serial port names. 
            string[] ports = SerialPort.GetPortNames();
            Console.WriteLine("The following serial ports were found:");

            // Display each port name to the console. 
            foreach (string port in ports)
            {
                Console.WriteLine(port);
            }
            Console.WriteLine("======================================");
            // set up serial port
            SerialPort _serialPort = new SerialPort();
            _serialPort.ReadTimeout = 10000;
            _serialPort.WriteTimeout = 500;
            _serialPort.BaudRate = 9600;

            while(!_serialPort.IsOpen){
                try
                {
                    Console.WriteLine("\nPlease type the name of your desired port:");
                    string choosePort = Console.ReadLine();
                    _serialPort.PortName = choosePort;
                    _serialPort.Open();
                    Console.WriteLine("======================================\n");
                    Console.WriteLine("Connected! Waiting for user commands...");
                } catch (System.IO.IOException e) {
                    Console.WriteLine("Error connecting with mbed ({0}).", e.Message);
                }
            }
            
            WebClient Client = new WebClient();
            Client.DownloadFile("http://104.131.69.134/commands.txt", "commands.txt");
            
            string[] commandsInit = File.ReadAllLines("commands.txt");
            string oldLastCommand = commandsInit[commandsInit.Length - 1];

            while (true)
            {
                Client.DownloadFile("http://104.131.69.134/commands.txt", "commands.txt");
                string[] commands = File.ReadAllLines("commands.txt");
                string newLastCommand = commands[commands.Length - 1];

                if(newLastCommand == oldLastCommand){
                    System.Threading.Thread.Sleep(1000);
                    continue;
                }
                else{

                    oldLastCommand = newLastCommand;
                    string[] splitCommand = newLastCommand.Split(',');

                    Console.WriteLine("\n========================= {0}", DateTime.Now.ToString("hh:mm:ss tt"));
                    Console.WriteLine(splitCommand[1]);
                    _serialPort.WriteLine(splitCommand[1]);

                    try
                    {
                        string message = _serialPort.ReadLine();
                        Console.WriteLine(message);
                    } catch (System.TimeoutException e) {
                        Console.WriteLine("    mbed did not respond ({0}).", e.Message);
                    }
                    
                    System.Threading.Thread.Sleep(500);
                }
            }
        }
    }
}
