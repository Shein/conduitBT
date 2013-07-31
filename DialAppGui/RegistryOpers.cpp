#include "RegistryOpers.h"


String^ RegistryOpers::Read (String ^KeyName)
{
	RegistryKey ^rk = baseRegistryKey;
	RegistryKey ^sk = rk->OpenSubKey(subKey);	// open a subKey as read-only

	if (!sk)
		return nullptr;

	try {
		return safe_cast<String^>(sk->GetValue(KeyName));
	}
	catch (Exception ^e) {
		ShowErrorMessage(e, "Reading registry " + KeyName);
		return nullptr;
	}
}


int RegistryOpers::ReadInt (String ^KeyName)
{
	RegistryKey ^rk = baseRegistryKey;
	RegistryKey ^sk = rk->OpenSubKey(subKey);	// open a subKey as read-only

	if (!sk)
		return 0;

	try {
		return safe_cast<int>(sk->GetValue(KeyName));
	}
	catch (Exception ^e) {
		ShowErrorMessage(e, "Reading registry " + KeyName);
		return 0;
	}
}


bool RegistryOpers::Write (String ^KeyName, Object ^Value)
{
	try	{
		RegistryKey ^rk = baseRegistryKey;
		RegistryKey ^sk = rk->CreateSubKey(subKey);	// create or open it if already exits
		sk->SetValue(KeyName, Value);
		return true;
	}
	catch (Exception ^e) {
		ShowErrorMessage(e, "Writing registry " + KeyName);
		return false;
	}
}


bool RegistryOpers::WriteInt (String ^KeyName, int Value)
{
	try	{
		RegistryKey ^rk = baseRegistryKey;
		RegistryKey ^sk = rk->CreateSubKey(subKey);	// create or open it if already exits
		sk->SetValue(KeyName, Value);
		return true;
	}
	catch (Exception ^e) {
		ShowErrorMessage(e, "Writing registry " + KeyName);
		return false;
	}
}


bool RegistryOpers::DeleteKey(String ^KeyName)
{
	try	{
		RegistryKey ^rk = baseRegistryKey;
		RegistryKey ^sk = rk->CreateSubKey(subKey);
		if (sk)
			sk->DeleteValue(KeyName);
		return true;
	}
	catch (Exception ^e) {
		ShowErrorMessage(e, "Deleting SubKey " + subKey);
		return false;
	}
}


bool RegistryOpers::DeleteSubKeyTree()
{
	try	{
		RegistryKey ^rk = baseRegistryKey;
		RegistryKey ^sk = rk->OpenSubKey(subKey);
		if (sk)
			rk->DeleteSubKeyTree(subKey);
		return true;
	}
	catch (Exception ^e) {
		ShowErrorMessage(e, "Deleting SubKey " + subKey);
		return false;
	}
}


int RegistryOpers::SubKeyCount()
{
	try	{
		RegistryKey ^rk = baseRegistryKey;
		RegistryKey ^sk = rk->OpenSubKey(subKey);

		if (sk)
			return sk->SubKeyCount;
		return 0;
	}
	catch (Exception ^e) {
		ShowErrorMessage(e, "Retrieving subkeys of " + subKey);
		return 0;
	}
}

int RegistryOpers::ValueCount()
{
	try	{
		RegistryKey ^rk = baseRegistryKey;
		RegistryKey ^sk = rk->OpenSubKey(subKey);
		if (sk)
			return sk->ValueCount;
		return 0;
	}
	catch (Exception ^e) {
		ShowErrorMessage(e, "Retrieving keys of " + subKey);
		return 0;
	}
}


void RegistryOpers::ShowErrorMessage(Exception ^e, String ^Title)
{
	if (showError)
		MessageBox::Show(e->Message, Title, MessageBoxButtons::OK, MessageBoxIcon::Error);
}

