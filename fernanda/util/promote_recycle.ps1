$shell = New-Object -ComObject 'Shell.Application'
if (!$args)
{
    $paths =
    (Get-Item -Path "C:\Dev\fernanda-dev\x64" -Force),
    (Get-Item -Path "C:\Dev\fernanda-dev\fernanda\x64" -Force),
    (Get-Item -Path "C:\Dev\fernanda-dev\.vs" -Force)
    ForEach ($path in $paths)
    {
        $shell.NameSpace(0).ParseName($path.FullName).InvokeVerb('delete')
    }
}
else
{
    ForEach ($argument in $args)
    {
        $paths = Get-ChildItem -Path "$argument" -Force -Exclude ".git"
        ForEach ($path in $paths)
        {
	        $shell.NameSpace(0).ParseName($path.FullName).InvokeVerb('delete')
        }
    }
}
