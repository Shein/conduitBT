#pragma once

#include "RegistryOpers.h"

using namespace System;
using namespace System::Runtime::InteropServices;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;


namespace DialAppGui {

	/// <summary>
	/// Summary for DialForm
	/// </summary>
	public ref class DialForm : public System::Windows::Forms::Form
	{
	public:
		static RegistryOpers^	Registry = gcnew RegistryOpers("Conduit\\HfpDialApp");
		static DialForm^		This;

	public:
		delegate void SetLabelDelegate	(String^ text);
		delegate void SetErrorDelegate	(String^ text1, String^ text2);
		delegate void SetDeviceDelegate	(String^ text1, array<String^>^ items);

	public:
		DialForm()
		{
			This = this;
			InitializeComponent();
		}

	public:
		array<String^>^  InitDevicesCombo();

		void SetStateName(String^ state)
		{
		   if (labelState->InvokeRequired) {
			  SetLabelDelegate^ action = gcnew SetLabelDelegate (this, &DialForm::SetStateName);
			  labelState->Invoke(action, state);
			  return;
		   }
		   labelState->Text = state;
		   tboxLog->AppendText("State changed: " + state + "\n");
		}

		void SetDeviceName(String^ device, array<String^>^ items)
		{
		   if (eboxDevice->InvokeRequired) {
			  SetDeviceDelegate^ action = gcnew SetDeviceDelegate (this, &DialForm::SetDeviceName);
			  eboxDevice->Invoke(action, device, items);
			  return;
		   }
		   eboxDevice->Items->Clear ();
		   eboxDevice->Items->AddRange (items);
		   eboxDeviceText   = device;
		   eboxDevice->Text = device;
		   if (device != "")
				tboxLog->AppendText("Device selected: " + device + "\n");
		}

		void SetHeadsetButtonName(String^ text)
		{
		   if (labelState->InvokeRequired) {
			  SetLabelDelegate^ action = gcnew SetLabelDelegate (this, &DialForm::SetHeadsetButtonName);
			  labelState->Invoke(action, text);
			  return;
		   }
		   btnHeadset->Text = text;
		   tboxLog->AppendText(text + "\n");
		}

		void SetError(String^ state, String^ error)
		{
		   if (labelState->InvokeRequired) {
			  SetErrorDelegate^ action = gcnew SetErrorDelegate (this, &DialForm::SetError);
			  labelState->Invoke(action, state, error);
			  return;
		   }
		   labelState->Text = state;
		   tboxLog->AppendText("Jump to state '" + state + "' after last ERROR = '" + error + "'\n");
		}

		void AddInfoMessage(String^ msg)
		{
		   if (labelState->InvokeRequired) {
			  SetLabelDelegate^ action = gcnew SetLabelDelegate (this, &DialForm::AddInfoMessage);
			  labelState->Invoke(action, msg);
			  return;
		   }
		   tboxLog->AppendText(msg + "\n");
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~DialForm()
		{
			if (components)
				delete components;
		}

		void DialForm_Load(Object ^sender, EventArgs ^e);
		void DialForm_FormClosing(Object ^sender, FormClosingEventArgs ^e);

		void btnClear_Click(Object ^sender, EventArgs ^e);
		void btnSelectDevice_Click(Object ^sender, EventArgs ^e);
		void btnForgetDevice_Click(Object ^sender, EventArgs ^e);
		void eboxDevice_Click(System::Object^ sender, System::EventArgs^ e);
		void btnCall_Click(Object ^sender, EventArgs ^e);
		void btnAnswer_Click(Object ^sender, EventArgs ^e);
		void btnEndCall_Click(Object ^sender, EventArgs ^e);
		void btnServCon_Click(Object ^sender, EventArgs ^e);
		void btnPcSound_Click(Object ^sender, EventArgs ^e);
		void btnConnect_Click(Object ^sender, EventArgs ^e);
		void btnDisconnect_Click(Object ^sender, EventArgs ^e);
		void chboxAutoServCon_Click(System::Object^  sender, System::EventArgs^ e);
		void btn1_Click(Object ^sender, EventArgs ^e);
		void btn2_Click(Object ^sender, EventArgs ^e);
		void btn3_Click(Object ^sender, EventArgs ^e);
		void btn4_Click(Object ^sender, EventArgs ^e);
		void btn5_Click(Object ^sender, EventArgs ^e);
		void btn6_Click(Object ^sender, EventArgs ^e);
		void btn7_Click(Object ^sender, EventArgs ^e);
		void btn8_Click(Object ^sender, EventArgs ^e);
		void btn9_Click(Object ^sender, EventArgs ^e);
		void button_diez_Click(Object ^sender, EventArgs ^e);
		void button_zero_Click(Object ^sender, EventArgs ^e);
		void button_star_Click(Object ^sender, EventArgs ^e);
		void button_SendAT_Click(Object ^sender, EventArgs ^e);
		void button_Hold_Click(Object ^sender, EventArgs ^e);
		// String^ utilities
		static cchar* String2Pchar (String ^str) { return (cchar*) Marshal::StringToHGlobalAnsi(str).ToPointer(); }
		static void   FreePchar	(cchar *str)	 { Marshal::FreeHGlobal((IntPtr)((void*)str)); }

		private: String^ eboxDeviceText;

		private: System::Windows::Forms::Label^ label1;
		private: System::Windows::Forms::Label^ label2;
		private: System::Windows::Forms::Label^ label3;
		private: System::Windows::Forms::Label^ label4;

		private: System::Windows::Forms::Label^ labelState;

		private: System::Windows::Forms::Button^ btnConnect;
		private: System::Windows::Forms::Button^ btnDisconnect;
		private: System::Windows::Forms::Button^ btnServCon;
		private: System::Windows::Forms::Button^ btnCall;
		private: System::Windows::Forms::Button^ btnAnswer;
		private: System::Windows::Forms::Button^ btnHeadset;
		private: System::Windows::Forms::Button^ btnEndCall;
		private: System::Windows::Forms::Button^ btnClear;
		private: System::Windows::Forms::Button^ btnSelectDevice;
		private: System::Windows::Forms::Button^ btnForgetDevice;

		private: System::Windows::Forms::ComboBox^ eboxDevice;
		private: System::Windows::Forms::TextBox^  eboxDialNumber;
		private: System::Windows::Forms::TextBox^  tboxLog;
		private: System::Windows::Forms::CheckBox^ chboxAutoServCon;

		private: System::Windows::Forms::Button^  button1;
		private: System::Windows::Forms::Button^  button2;
		private: System::Windows::Forms::Button^  button3;
		private: System::Windows::Forms::Button^  button4;
		private: System::Windows::Forms::Button^  button5;
		private: System::Windows::Forms::Button^  button6;
		private: System::Windows::Forms::Button^  button7;
		private: System::Windows::Forms::Button^  button8;
		private: System::Windows::Forms::Button^  button9;
		private: System::Windows::Forms::Button^  button_diez;
		private: System::Windows::Forms::Button^  button_zero;
		private: System::Windows::Forms::Button^  button_star;
		private: System::Windows::Forms::TextBox^  textBox_AT_command;
		private: System::Windows::Forms::Button^  button_SendAT;
		private: System::Windows::Forms::Button^  button_Hold;

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->label1 = (gcnew System::Windows::Forms::Label());
			this->label2 = (gcnew System::Windows::Forms::Label());
			this->label3 = (gcnew System::Windows::Forms::Label());
			this->label4 = (gcnew System::Windows::Forms::Label());
			this->btnClear = (gcnew System::Windows::Forms::Button());
			this->btnEndCall = (gcnew System::Windows::Forms::Button());
			this->btnCall = (gcnew System::Windows::Forms::Button());
			this->eboxDialNumber = (gcnew System::Windows::Forms::TextBox());
			this->btnDisconnect = (gcnew System::Windows::Forms::Button());
			this->btnConnect = (gcnew System::Windows::Forms::Button());
			this->labelState = (gcnew System::Windows::Forms::Label());
			this->tboxLog = (gcnew System::Windows::Forms::TextBox());
			this->btnHeadset = (gcnew System::Windows::Forms::Button());
			this->chboxAutoServCon = (gcnew System::Windows::Forms::CheckBox());
			this->btnServCon = (gcnew System::Windows::Forms::Button());
			this->btnSelectDevice = (gcnew System::Windows::Forms::Button());
			this->eboxDevice = (gcnew System::Windows::Forms::ComboBox());
			this->btnForgetDevice = (gcnew System::Windows::Forms::Button());
			this->btnAnswer = (gcnew System::Windows::Forms::Button());
			this->button1 = (gcnew System::Windows::Forms::Button());
			this->button2 = (gcnew System::Windows::Forms::Button());
			this->button3 = (gcnew System::Windows::Forms::Button());
			this->button4 = (gcnew System::Windows::Forms::Button());
			this->button5 = (gcnew System::Windows::Forms::Button());
			this->button6 = (gcnew System::Windows::Forms::Button());
			this->button7 = (gcnew System::Windows::Forms::Button());
			this->button8 = (gcnew System::Windows::Forms::Button());
			this->button9 = (gcnew System::Windows::Forms::Button());
			this->button_diez = (gcnew System::Windows::Forms::Button());
			this->button_zero = (gcnew System::Windows::Forms::Button());
			this->button_star = (gcnew System::Windows::Forms::Button());
			this->textBox_AT_command = (gcnew System::Windows::Forms::TextBox());
			this->button_SendAT = (gcnew System::Windows::Forms::Button());
			this->button_Hold = (gcnew System::Windows::Forms::Button());
			this->SuspendLayout();
			// 
			// label1
			// 
			this->label1->BackColor = System::Drawing::Color::Transparent;
			this->label1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 11.25F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(204)));
			this->label1->ForeColor = System::Drawing::Color::Black;
			this->label1->Location = System::Drawing::Point(14, 12);
			this->label1->Name = L"label1";
			this->label1->Size = System::Drawing::Size(55, 24);
			this->label1->TabIndex = 13;
			this->label1->Text = L"De&vice:";
			// 
			// label2
			// 
			this->label2->BackColor = System::Drawing::Color::Transparent;
			this->label2->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 11.25F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(204)));
			this->label2->ForeColor = System::Drawing::Color::Black;
			this->label2->Location = System::Drawing::Point(17, 94);
			this->label2->Name = L"label2";
			this->label2->Size = System::Drawing::Size(52, 24);
			this->label2->TabIndex = 13;
			this->label2->Text = L"Dial #";
			// 
			// label3
			// 
			this->label3->BackColor = System::Drawing::Color::Transparent;
			this->label3->ForeColor = System::Drawing::Color::Black;
			this->label3->Location = System::Drawing::Point(17, 255);
			this->label3->Name = L"label3";
			this->label3->Size = System::Drawing::Size(52, 18);
			this->label3->TabIndex = 14;
			this->label3->Text = L"Log:";
			// 
			// label4
			// 
			this->label4->AutoSize = true;
			this->label4->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 11.25F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(204)));
			this->label4->Location = System::Drawing::Point(17, 173);
			this->label4->Name = L"label4";
			this->label4->Size = System::Drawing::Size(46, 18);
			this->label4->TabIndex = 15;
			this->label4->Text = L"State:";
			// 
			// btnClear
			// 
			this->btnClear->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 6.75F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(204)));
			this->btnClear->Location = System::Drawing::Point(539, 247);
			this->btnClear->Name = L"btnClear";
			this->btnClear->Size = System::Drawing::Size(44, 23);
			this->btnClear->TabIndex = 5;
			this->btnClear->Text = L"Clear";
			this->btnClear->Click += gcnew System::EventHandler(this, &DialForm::btnClear_Click);
			// 
			// btnEndCall
			// 
			this->btnEndCall->Location = System::Drawing::Point(70, 124);
			this->btnEndCall->Name = L"btnEndCall";
			this->btnEndCall->Size = System::Drawing::Size(216, 21);
			this->btnEndCall->TabIndex = 4;
			this->btnEndCall->Text = L"&End Call";
			this->btnEndCall->Click += gcnew System::EventHandler(this, &DialForm::btnEndCall_Click);
			// 
			// btnCall
			// 
			this->btnCall->Location = System::Drawing::Point(292, 94);
			this->btnCall->Name = L"btnCall";
			this->btnCall->Size = System::Drawing::Size(83, 24);
			this->btnCall->TabIndex = 3;
			this->btnCall->Text = L"&Call";
			this->btnCall->Click += gcnew System::EventHandler(this, &DialForm::btnCall_Click);
			// 
			// eboxDialNumber
			// 
			this->eboxDialNumber->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 11.25F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(204)));
			this->eboxDialNumber->Location = System::Drawing::Point(70, 94);
			this->eboxDialNumber->MaxLength = 40;
			this->eboxDialNumber->Name = L"eboxDialNumber";
			this->eboxDialNumber->Size = System::Drawing::Size(216, 24);
			this->eboxDialNumber->TabIndex = 2;
			this->eboxDialNumber->Text = L"1-800-335-166";
			// 
			// btnDisconnect
			// 
			this->btnDisconnect->Location = System::Drawing::Point(181, 40);
			this->btnDisconnect->Name = L"btnDisconnect";
			this->btnDisconnect->Size = System::Drawing::Size(105, 24);
			this->btnDisconnect->TabIndex = 4;
			this->btnDisconnect->Text = L"Disconnect";
			this->btnDisconnect->Click += gcnew System::EventHandler(this, &DialForm::btnDisconnect_Click);
			// 
			// btnConnect
			// 
			this->btnConnect->Location = System::Drawing::Point(70, 40);
			this->btnConnect->Name = L"btnConnect";
			this->btnConnect->Size = System::Drawing::Size(105, 24);
			this->btnConnect->TabIndex = 3;
			this->btnConnect->Text = L"Connect";
			this->btnConnect->Click += gcnew System::EventHandler(this, &DialForm::btnConnect_Click);
			// 
			// labelState
			// 
			this->labelState->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left) 
				| System::Windows::Forms::AnchorStyles::Right));
			this->labelState->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 11.25F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(204)));
			this->labelState->Location = System::Drawing::Point(67, 173);
			this->labelState->Name = L"labelState";
			this->labelState->Size = System::Drawing::Size(219, 26);
			this->labelState->TabIndex = 16;
			this->labelState->Text = L"IdleNoDevice";
			// 
			// tboxLog
			// 
			this->tboxLog->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom) 
				| System::Windows::Forms::AnchorStyles::Left) 
				| System::Windows::Forms::AnchorStyles::Right));
			this->tboxLog->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(204)));
			this->tboxLog->Location = System::Drawing::Point(12, 276);
			this->tboxLog->Multiline = true;
			this->tboxLog->Name = L"tboxLog";
			this->tboxLog->ScrollBars = System::Windows::Forms::ScrollBars::Both;
			this->tboxLog->Size = System::Drawing::Size(581, 306);
			this->tboxLog->TabIndex = 19;
			// 
			// btnHeadset
			// 
			this->btnHeadset->Location = System::Drawing::Point(292, 121);
			this->btnHeadset->Name = L"btnHeadset";
			this->btnHeadset->Size = System::Drawing::Size(83, 24);
			this->btnHeadset->TabIndex = 3;
			this->btnHeadset->Text = L"PC Sound Off";
			this->btnHeadset->Click += gcnew System::EventHandler(this, &DialForm::btnPcSound_Click);
			// 
			// chboxAutoServCon
			// 
			this->chboxAutoServCon->AutoSize = true;
			this->chboxAutoServCon->Checked = true;
			this->chboxAutoServCon->CheckState = System::Windows::Forms::CheckState::Checked;
			this->chboxAutoServCon->Location = System::Drawing::Point(381, 40);
			this->chboxAutoServCon->Name = L"chboxAutoServCon";
			this->chboxAutoServCon->Size = System::Drawing::Size(202, 17);
			this->chboxAutoServCon->TabIndex = 20;
			this->chboxAutoServCon->Text = L"Auto Service Level Connection setup";
			this->chboxAutoServCon->UseVisualStyleBackColor = true;
			this->chboxAutoServCon->Visible = false;
			this->chboxAutoServCon->Click += gcnew System::EventHandler(this, &DialForm::chboxAutoServCon_Click);
			// 
			// btnServCon
			// 
			this->btnServCon->Enabled = false;
			this->btnServCon->Location = System::Drawing::Point(292, 40);
			this->btnServCon->Name = L"btnServCon";
			this->btnServCon->Size = System::Drawing::Size(83, 24);
			this->btnServCon->TabIndex = 21;
			this->btnServCon->Text = L"Service Con";
			this->btnServCon->Click += gcnew System::EventHandler(this, &DialForm::btnServCon_Click);
			// 
			// btnSelectDevice
			// 
			this->btnSelectDevice->Location = System::Drawing::Point(292, 10);
			this->btnSelectDevice->Name = L"btnSelectDevice";
			this->btnSelectDevice->Size = System::Drawing::Size(83, 24);
			this->btnSelectDevice->TabIndex = 3;
			this->btnSelectDevice->Text = L"Select Device";
			this->btnSelectDevice->Click += gcnew System::EventHandler(this, &DialForm::btnSelectDevice_Click);
			// 
			// eboxDevice
			// 
			this->eboxDevice->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->eboxDevice->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 11.25F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(204)));
			this->eboxDevice->Location = System::Drawing::Point(70, 12);
			this->eboxDevice->Name = L"eboxDevice";
			this->eboxDevice->Size = System::Drawing::Size(216, 26);
			this->eboxDevice->TabIndex = 2;
			this->eboxDevice->SelectionChangeCommitted += gcnew System::EventHandler(this, &DialForm::eboxDevice_Click);
			// 
			// btnForgetDevice
			// 
			this->btnForgetDevice->Location = System::Drawing::Point(381, 10);
			this->btnForgetDevice->Name = L"btnForgetDevice";
			this->btnForgetDevice->Size = System::Drawing::Size(83, 24);
			this->btnForgetDevice->TabIndex = 3;
			this->btnForgetDevice->Text = L"Forget Device";
			this->btnForgetDevice->Click += gcnew System::EventHandler(this, &DialForm::btnForgetDevice_Click);
			// 
			// btnAnswer
			// 
			this->btnAnswer->Location = System::Drawing::Point(381, 94);
			this->btnAnswer->Name = L"btnAnswer";
			this->btnAnswer->Size = System::Drawing::Size(83, 24);
			this->btnAnswer->TabIndex = 3;
			this->btnAnswer->Text = L"&Answer";
			this->btnAnswer->Click += gcnew System::EventHandler(this, &DialForm::btnAnswer_Click);
			// 
			// button1
			// 
			this->button1->Location = System::Drawing::Point(292, 219);
			this->button1->Name = L"button1";
			this->button1->Size = System::Drawing::Size(27, 23);
			this->button1->TabIndex = 23;
			this->button1->Text = L"1";
			this->button1->UseVisualStyleBackColor = true;
			this->button1->Click += gcnew System::EventHandler(this, &DialForm::btn1_Click);
			// 
			// button2
			// 
			this->button2->Location = System::Drawing::Point(320, 219);
			this->button2->Name = L"button2";
			this->button2->Size = System::Drawing::Size(27, 23);
			this->button2->TabIndex = 24;
			this->button2->Text = L"2";
			this->button2->UseVisualStyleBackColor = true;
			this->button2->Click += gcnew System::EventHandler(this, &DialForm::btn2_Click);
			// 
			// button3
			// 
			this->button3->Location = System::Drawing::Point(348, 219);
			this->button3->Name = L"button3";
			this->button3->Size = System::Drawing::Size(27, 23);
			this->button3->TabIndex = 25;
			this->button3->Text = L"3";
			this->button3->UseVisualStyleBackColor = true;
			this->button3->Click += gcnew System::EventHandler(this, &DialForm::btn3_Click);
			// 
			// button4
			// 
			this->button4->Location = System::Drawing::Point(292, 193);
			this->button4->Name = L"button4";
			this->button4->Size = System::Drawing::Size(27, 23);
			this->button4->TabIndex = 28;
			this->button4->Text = L"4";
			this->button4->UseVisualStyleBackColor = true;
			this->button4->Click += gcnew System::EventHandler(this, &DialForm::btn4_Click);
			// 
			// button5
			// 
			this->button5->Location = System::Drawing::Point(320, 193);
			this->button5->Name = L"button5";
			this->button5->Size = System::Drawing::Size(27, 23);
			this->button5->TabIndex = 27;
			this->button5->Text = L"5";
			this->button5->UseVisualStyleBackColor = true;
			this->button5->Click += gcnew System::EventHandler(this, &DialForm::btn5_Click);
			// 
			// button6
			// 
			this->button6->Location = System::Drawing::Point(348, 193);
			this->button6->Name = L"button6";
			this->button6->Size = System::Drawing::Size(27, 23);
			this->button6->TabIndex = 26;
			this->button6->Text = L"6";
			this->button6->UseVisualStyleBackColor = true;
			this->button6->Click += gcnew System::EventHandler(this, &DialForm::btn6_Click);
			// 
			// button7
			// 
			this->button7->Location = System::Drawing::Point(292, 168);
			this->button7->Name = L"button7";
			this->button7->Size = System::Drawing::Size(27, 23);
			this->button7->TabIndex = 31;
			this->button7->Text = L"7";
			this->button7->UseVisualStyleBackColor = true;
			this->button7->Click += gcnew System::EventHandler(this, &DialForm::btn7_Click);
			// 
			// button8
			// 
			this->button8->Location = System::Drawing::Point(320, 168);
			this->button8->Name = L"button8";
			this->button8->Size = System::Drawing::Size(27, 23);
			this->button8->TabIndex = 30;
			this->button8->Text = L"8";
			this->button8->UseVisualStyleBackColor = true;
			this->button8->Click += gcnew System::EventHandler(this, &DialForm::btn8_Click);
			// 
			// button9
			// 
			this->button9->Location = System::Drawing::Point(348, 168);
			this->button9->Name = L"button9";
			this->button9->Size = System::Drawing::Size(27, 23);
			this->button9->TabIndex = 29;
			this->button9->Text = L"9";
			this->button9->UseVisualStyleBackColor = true;
			this->button9->Click += gcnew System::EventHandler(this, &DialForm::btn9_Click);
			// 
			// button_diez
			// 
			this->button_diez->Location = System::Drawing::Point(348, 244);
			this->button_diez->Name = L"button_diez";
			this->button_diez->Size = System::Drawing::Size(27, 23);
			this->button_diez->TabIndex = 34;
			this->button_diez->Text = L"#";
			this->button_diez->UseVisualStyleBackColor = true;
			this->button_diez->Click += gcnew System::EventHandler(this, &DialForm::button_diez_Click);
			// 
			// button_zero
			// 
			this->button_zero->Location = System::Drawing::Point(320, 244);
			this->button_zero->Name = L"button_zero";
			this->button_zero->Size = System::Drawing::Size(27, 23);
			this->button_zero->TabIndex = 33;
			this->button_zero->Text = L"0";
			this->button_zero->UseVisualStyleBackColor = true;
			this->button_zero->Click += gcnew System::EventHandler(this, &DialForm::button_zero_Click);
			// 
			// button_star
			// 
			this->button_star->Location = System::Drawing::Point(292, 244);
			this->button_star->Name = L"button_star";
			this->button_star->Size = System::Drawing::Size(27, 23);
			this->button_star->TabIndex = 32;
			this->button_star->Text = L"*";
			this->button_star->UseVisualStyleBackColor = true;
			this->button_star->Click += gcnew System::EventHandler(this, &DialForm::button_star_Click);
			// 
			// textBox_AT_command
			// 
			this->textBox_AT_command->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 11.25F, System::Drawing::FontStyle::Regular, 
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(204)));
			this->textBox_AT_command->Location = System::Drawing::Point(477, 94);
			this->textBox_AT_command->Name = L"textBox_AT_command";
			this->textBox_AT_command->Size = System::Drawing::Size(116, 24);
			this->textBox_AT_command->TabIndex = 38;
			// 
			// button_SendAT
			// 
			this->button_SendAT->Location = System::Drawing::Point(527, 124);
			this->button_SendAT->Name = L"button_SendAT";
			this->button_SendAT->Size = System::Drawing::Size(66, 23);
			this->button_SendAT->TabIndex = 39;
			this->button_SendAT->Text = L"Send AT";
			this->button_SendAT->UseVisualStyleBackColor = true;
			this->button_SendAT->Click += gcnew System::EventHandler(this, &DialForm::button_SendAT_Click);
			// 
			// button_Hold
			// 
			this->button_Hold->Location = System::Drawing::Point(381, 121);
			this->button_Hold->Name = L"button_Hold";
			this->button_Hold->Size = System::Drawing::Size(83, 23);
			this->button_Hold->TabIndex = 40;
			this->button_Hold->Text = L"Hold/Switch";
			this->button_Hold->UseVisualStyleBackColor = true;
			this->button_Hold->Click += gcnew System::EventHandler(this, &DialForm::button_Hold_Click);
			// 
			// DialForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(605, 594);
			this->Controls->Add(this->button_Hold);
			this->Controls->Add(this->button_SendAT);
			this->Controls->Add(this->textBox_AT_command);
			this->Controls->Add(this->button_diez);
			this->Controls->Add(this->button_zero);
			this->Controls->Add(this->button_star);
			this->Controls->Add(this->button7);
			this->Controls->Add(this->button8);
			this->Controls->Add(this->button9);
			this->Controls->Add(this->button4);
			this->Controls->Add(this->button5);
			this->Controls->Add(this->button6);
			this->Controls->Add(this->button3);
			this->Controls->Add(this->button2);
			this->Controls->Add(this->button1);
			this->Controls->Add(this->btnServCon);
			this->Controls->Add(this->chboxAutoServCon);
			this->Controls->Add(this->tboxLog);
			this->Controls->Add(this->labelState);
			this->Controls->Add(this->label4);
			this->Controls->Add(this->label3);
			this->Controls->Add(this->label1);
			this->Controls->Add(this->label2);
			this->Controls->Add(this->btnClear);
			this->Controls->Add(this->btnDisconnect);
			this->Controls->Add(this->btnEndCall);
			this->Controls->Add(this->btnForgetDevice);
			this->Controls->Add(this->btnSelectDevice);
			this->Controls->Add(this->btnConnect);
			this->Controls->Add(this->btnHeadset);
			this->Controls->Add(this->btnAnswer);
			this->Controls->Add(this->btnCall);
			this->Controls->Add(this->eboxDevice);
			this->Controls->Add(this->eboxDialNumber);
			this->Name = L"DialForm";
			this->Text = L"HFP Dial Pad";
			this->FormClosing += gcnew System::Windows::Forms::FormClosingEventHandler(this, &DialForm::DialForm_FormClosing);
			this->Load += gcnew System::EventHandler(this, &DialForm::DialForm_Load);
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
	};
}
