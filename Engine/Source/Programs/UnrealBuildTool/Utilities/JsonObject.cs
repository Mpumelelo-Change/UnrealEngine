﻿using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace UnrealBuildTool
{
	public class JsonObject
	{
		Dictionary<string, object> RawObject;

		public JsonObject(Dictionary<string, object> InRawObject)
		{
			RawObject = new Dictionary<string,object>(InRawObject, StringComparer.InvariantCultureIgnoreCase);
		}

		public static JsonObject FromFile(string FileName)
		{
			string Text = File.ReadAllText(FileName);
			Dictionary<string, object> CaseSensitiveRawObject = fastJSON.JSON.Instance.ToObject<Dictionary<string, object>>(Text);
			return new JsonObject(CaseSensitiveRawObject);
		}

		public string GetStringField(string FieldName)
		{
			string StringValue;
			if(!TryGetStringField(FieldName, out StringValue))
			{
				throw new BuildException("Missing or invalid '{0}' field", FieldName);
			}
			return StringValue;
		}

		public bool TryGetStringField(string FieldName, out string Result)
		{
			object RawValue;
			if(RawObject.TryGetValue(FieldName, out RawValue) && (RawValue is string))
			{
				Result = (string)RawValue;
				return true;
			}
			else
			{
				Result = null;
				return false;
			}
		}

		public bool TryGetStringArrayField(string FieldName, out string[] Result)
		{
			object RawValue;
			if(RawObject.TryGetValue(FieldName, out RawValue) && (RawValue is object[]) && ((object[])RawValue).All(x => x is string))
			{
				Result = Array.ConvertAll((object[])RawValue, x => (string)x);
				return true;
			}
			else
			{
				Result = null;
				return false;
			}
		}

		public bool TryGetBoolField(string FieldName, out bool Result)
		{
			object RawValue;
			if(RawObject.TryGetValue(FieldName, out RawValue) && (RawValue is Boolean))
			{
				Result = (bool)RawValue;
				return true;
			}
			else
			{
				Result = false;
				return false;
			}
		}

		public bool TryGetNumericField(string FieldName, out int Result)
		{
			object RawValue;
			if(!RawObject.TryGetValue(FieldName, out RawValue) || !int.TryParse(RawValue.ToString(), out Result))
			{
				Result = 0;
				return false;
			}
			return true;
		}

		public T GetEnumField<T>(string FieldName) where T : struct
		{
			T EnumValue;
			if(!TryGetEnumField(FieldName, out EnumValue))
			{
				throw new BuildException("Missing or invalid '{0}' field", FieldName);
			}
			return EnumValue;
		}

		public bool TryGetEnumField<T>(string FieldName, out T Result) where T : struct
		{
			string StringValue;
			if(!TryGetStringField(FieldName, out StringValue) || !Enum.TryParse<T>(StringValue, true, out Result))
			{
				Result = default(T);
				return false;
			}
			return true;
		}

		public bool TryGetEnumArrayField<T>(string FieldName, out T[] Result) where T : struct
		{
			string[] StringValues;
			if(!TryGetStringArrayField(FieldName, out StringValues))
			{
				Result = null;
				return false;
			}

			T[] EnumValues = new T[StringValues.Length];
			for(int Idx = 0; Idx < StringValues.Length; Idx++)
			{
				if(!Enum.TryParse<T>(StringValues[Idx], true, out EnumValues[Idx]))
				{
					Result = null;
					return false;
				}
			}

			Result = EnumValues;
			return true;
		}

		public bool TryGetObjectArrayField(string FieldName, out JsonObject[] Result)
		{
			object RawValue;
			if(RawObject.TryGetValue(FieldName, out RawValue) && (RawValue is object[]) && ((object[])RawValue).All(x => x is Dictionary<string, object>))
			{
				Result = Array.ConvertAll((object[])RawValue, x => new JsonObject((Dictionary<string, object>)x));
				return true;
			}
			else
			{
				Result = null;
				return false;
			}
		}
	}
}
