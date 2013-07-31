#pragma once

using namespace System;
using namespace Microsoft::Win32;
using namespace System::Windows::Forms;


/// <summary>
/// An useful class to read/write/delete/count registry keys
/// </summary>
public ref class RegistryOpers
{
  public:
	RegistryOpers()
	{
		baseRegistryKey = Registry::LocalMachine;
		subKey			= "SOFTWARE\\" + Application::ProductName;
	}

	RegistryOpers(String ^ swsubkey)
	{
		baseRegistryKey = Registry::LocalMachine;
		subKey			= "SOFTWARE\\" + swsubkey;
	}

  private:
	bool showError;

	/// <summary>
	/// A property to show or hide error messages 
	/// (default = false)
	/// </summary>
  public:
	property bool ShowError
	{
		bool get()				{ return showError;  }
		void set(bool value)	{ showError = value; }
	}

  private:
	String ^subKey;

	/// <summary>
	/// A property to set the SubKey value
	/// (default = "SOFTWARE\\" + Application.ProductName.ToUpper())
	/// </summary>
  public:
	property String ^SubKey
	{
		String ^get()				{ return subKey;  }
		void set(String ^value)		{ subKey = value; }
	}

  private:
	RegistryKey ^baseRegistryKey;

	/// <summary>
	/// A property to set the BaseRegistryKey value.
	/// (default = Registry.LocalMachine)
	/// </summary>
  public:
	property RegistryKey ^BaseRegistryKey
	{
		RegistryKey ^get()				{ return baseRegistryKey;  }
		void set(RegistryKey ^value)	{ baseRegistryKey = value; }
	}


  public:
	/// <summary>
	/// To read a registry key.
	/// input: KeyName (string)
	/// output: value (string) 
	/// </summary>
	String ^Read(String ^KeyName);

	int ReadInt(String ^KeyName);


	/// <summary>
	/// To write into a registry key.
	/// input: KeyName (string) , Value (object)
	/// output: true or false 
	/// </summary>
	bool Write(String ^KeyName, Object ^Value);

	bool WriteInt(String ^KeyName, int Value);


	/// <summary>
	/// To delete a registry key.
	/// input: KeyName (string)
	/// output: true or false 
	/// </summary>
	bool DeleteKey(String ^KeyName);


	/// <summary>
	/// To delete a sub key and any child.
	/// input: void
	/// output: true or false 
	/// </summary>
	bool DeleteSubKeyTree();

	/// <summary>
	/// Retrieve the count of subkeys at the current key.
	/// input: void
	/// output: number of subkeys
	/// </summary>
	int SubKeyCount();


	/// <summary>
	/// Retrieve the count of values in the key.
	/// input: void
	/// output: number of keys
	/// </summary>
	int ValueCount();

	/* **************************************************************************
		* **************************************************************************/

  private:
	void ShowErrorMessage(Exception ^e, String ^Title);

};
