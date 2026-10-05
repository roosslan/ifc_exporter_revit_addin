using System;
using System.IO;
using System.Text;
using ifc_exporter;
using Xunit;

namespace ifc_exporter.tests
{
    /* Проверки разбора INI-файла в UTF-8 (замена GetPrivateProfileString) */
    public sealed class IniSimpleTests : IDisposable
    {
        private readonly string dir_;
        private readonly string path_;

        public IniSimpleTests()
        {
            dir_ = Path.Combine(Path.GetTempPath(), "ifc_exporter_tests_" + Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(dir_);
            path_ = Path.Combine(dir_, "test.inf");
        }

        public void Dispose()
        {
            Directory.Delete(dir_, true);
        }

        private void WriteIni(string content, bool bom = false)
        {
            File.WriteAllText(path_, content, new UTF8Encoding(bom));
        }

        private string ReadIni()
        {
            return File.ReadAllText(path_, Encoding.UTF8);
        }

        [Fact]
        public void ReadString_ReturnsValue()
        {
            WriteIni("[ControlFlags]\r\nEnabled=true\r\nTime=01.10.2026 10:00\r\n");
            var ini = new ini_simple(path_);

            Assert.Equal("true", ini.read_string("ControlFlags", "Enabled"));
            Assert.Equal("01.10.2026 10:00", ini.read_string("ControlFlags", "Time"));
        }

        [Fact]
        public void ReadString_IgnoresCaseOfSectionAndKey()
        {
            WriteIni("[ControlFlags]\nEnabled=true\n");
            var ini = new ini_simple(path_);

            Assert.Equal("true", ini.read_string("controlflags", "ENABLED"));
        }

        [Fact]
        public void ReadString_TrimsSpacesAndQuotes()
        {
            WriteIni("[S]\n  Key  =  \"value with spaces\"  \n");
            var ini = new ini_simple(path_);

            Assert.Equal("value with spaces", ini.read_string("S", "Key"));
        }

        [Fact]
        public void ReadString_ReturnsEmptyForMissingKeySectionOrFile()
        {
            WriteIni("[S]\nKey=1\n");
            var ini = new ini_simple(path_);

            Assert.Equal(string.Empty, ini.read_string("S", "Other"));
            Assert.Equal(string.Empty, ini.read_string("Other", "Key"));
            Assert.Equal(string.Empty, new ini_simple(Path.Combine(dir_, "missing.inf")).read_string("S", "Key"));
        }

        [Fact]
        public void ReadString_DoesNotTakeKeyFromAnotherSection()
        {
            WriteIni("[A]\nKey=a\n[B]\nKey=b\n");
            var ini = new ini_simple(path_);

            Assert.Equal("a", ini.read_string("A", "Key"));
            Assert.Equal("b", ini.read_string("B", "Key"));
        }

        [Fact]
        public void ReadString_SkipsComments()
        {
            WriteIni("[S]\n; Key=commented\n# Key=commented\nKey=real\n");
            var ini = new ini_simple(path_);

            Assert.Equal("real", ini.read_string("S", "Key"));
        }

        [Theory]
        [InlineData(false)]
        [InlineData(true)]
        public void ReadString_ReadsCyrillicInUtf8WithAndWithoutBom(bool bom)
        {
            WriteIni("[DestinationDirs]\nDefaultDestDir=D:\\Выгрузка\\Объект 1\n", bom);
            var ini = new ini_simple(path_);

            Assert.Equal("D:\\Выгрузка\\Объект 1", ini.read_string("DestinationDirs", "DefaultDestDir"));
        }

        [Fact]
        public void ReadString_IsNotLimitedTo255Characters()
        {
            var value = new string('x', 1000);
            WriteIni("[S]\nKey=" + value + "\n");
            var ini = new ini_simple(path_);

            Assert.Equal(value, ini.read_string("S", "Key"));
        }

        [Fact]
        public void ReadString_SplitsOnFirstEqualsSign()
        {
            WriteIni("[S]\nKey=a=b\n");
            var ini = new ini_simple(path_);

            Assert.Equal("a=b", ini.read_string("S", "Key"));
        }

        [Fact]
        public void WriteString_ReplacesExistingValueAndKeepsOtherLines()
        {
            WriteIni("[ControlFlags]\nEnabled=true\nTime=1\n\n[RVT]\nIFC=true\n");
            var ini = new ini_simple(path_);

            ini.write_string("ControlFlags", "Enabled", "false");

            Assert.Equal("false", ini.read_string("ControlFlags", "Enabled"));
            Assert.Equal("1", ini.read_string("ControlFlags", "Time"));
            Assert.Equal("true", ini.read_string("RVT", "IFC"));
        }

        [Fact]
        public void WriteString_AddsKeyToEndOfExistingSection()
        {
            WriteIni("[A]\nX=1\n\n[B]\nY=2\n");
            var ini = new ini_simple(path_);

            ini.write_string("A", "Z", "3");

            Assert.Equal("3", ini.read_string("A", "Z"));
            Assert.Equal(string.Empty, ini.read_string("B", "Z"));
            Assert.True(ReadIni().IndexOf("Z=3") < ReadIni().IndexOf("[B]"));
        }

        [Fact]
        public void WriteString_AddsMissingSectionAndCreatesMissingFile()
        {
            var ini = new ini_simple(path_);

            ini.write_string("New", "Key", "Значение");

            Assert.True(File.Exists(path_));
            Assert.Equal("Значение", ini.read_string("New", "Key"));
        }

        [Fact]
        public void WriteString_WritesUtf8WithoutBomAndLeavesNoTempFile()
        {
            WriteIni("[S]\nKey=1\n", bom: true);
            var ini = new ini_simple(path_);

            ini.write_string("S", "Key", "2");

            var bytes = File.ReadAllBytes(path_);
            Assert.False(bytes.Length >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF);
            Assert.False(File.Exists(path_ + ".tmp"));
        }

        [Fact]
        public void DeleteKey_RemovesOnlyThatKey()
        {
            WriteIni("[S]\nA=1\nB=2\n[T]\nA=3\n");
            var ini = new ini_simple(path_);

            ini.delete_key("S", "A");

            Assert.False(ini.key_exists("S", "A"));
            Assert.Equal("2", ini.read_string("S", "B"));
            Assert.Equal("3", ini.read_string("T", "A"));
        }

        [Fact]
        public void DeleteSection_RemovesSectionWithKeys()
        {
            WriteIni("[S]\nA=1\n[T]\nB=2\n");
            var ini = new ini_simple(path_);

            ini.delete_section("S");

            Assert.Null(ini.read_section("S"));
            Assert.Equal("2", ini.read_string("T", "B"));
        }

        [Fact]
        public void ReadSection_ReturnsPairsWithoutCommentsAndEmptyLines()
        {
            WriteIni("[SourceDisksFiles]\nC:\\a.rvt=true\n\n; comment\nC:\\b.rvt = false\n[Next]\nX=1\n");
            var ini = new ini_simple(path_);

            Assert.Equal(new[] { "C:\\a.rvt=true", "C:\\b.rvt=false" }, ini.read_section("SourceDisksFiles"));
        }

        [Fact]
        public void KeyExists_IsFalseForEmptyValue()
        {
            WriteIni("[S]\nEmpty=\nFull=1\n");
            var ini = new ini_simple(path_);

            Assert.False(ini.key_exists("S", "Empty"));
            Assert.True(ini.key_exists("S", "Full"));
        }
    }
}
